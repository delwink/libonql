/*
 *  Structured Query Object Notation (SQON) - C API
 *  Copyright (C) 2015 Delwink, LLC
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published by
 *  the Free Software Foundation, version 3 only.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jansson.h>
#include <mysql.h>

#include "internal.h"

#define PERM_GRANT 1
#define PERM_REVOKE 2

int
res_to_json (uint8_t type, void *res, char **out, const char *pk)
{
  int rc = 0;
  bool arr = (NULL == pk || !strcmp (pk, ""));
  json_t *root, *jsonrow;
  size_t num_fields, i;
  char *vpk = NULL;
  union fields fields;
  union row row;

  if (arr)
    root = json_array ();
  else
    root = json_object ();
  if (NULL == root)
    return SQON_MEMORYERROR;

  switch (type)
    {
    case SQON_DBCONN_MYSQL:
      num_fields = mysql_num_fields (res);
      if (!num_fields)
	{
	  rc = SQON_NOCOLUMNS;
	  break;
	}

      fields.mysql = mysql_fetch_fields (res);
      if (!arr)
	{
	  for (i = 0; i < num_fields; ++i)
	    {
	      if (strcmp (fields.mysql[i].name, pk))
		{
		  rc = SQON_NOPK;
		}
	      else
		{
		  rc = 0;
		  break;
		}
	    }
	  if (rc)
	    break;
	}

      while ((row.mysql = mysql_fetch_row (res)))
	{
	  jsonrow = json_object ();
	  if (NULL == jsonrow)
	    {
	      rc = SQON_MEMORYERROR;
	      break;
	    }

	  for (i = 0; i < num_fields; ++i)
	    {
	      if (arr)
		rc = json_object_set_new (jsonrow, fields.mysql[i].name,
					  json_string (row.mysql[i]));
	      else if (strcmp (fields.mysql[i].name, pk))
		rc = json_object_set_new (jsonrow, fields.mysql[i].name,
					  json_string (row.mysql[i]));
	      else
		vpk = row.mysql[i];
	      if (rc)
		{
		  rc = SQON_MEMORYERROR;
		  break;
		}
	    }

	  if (rc)
	    {
	      json_decref (jsonrow);
	      break;
	    }

	  if (arr)
	    {
	      json_array_append (root, jsonrow);
	    }
	  else if (NULL == json_object_get (root, vpk))
	    {
	      json_object_set (root, vpk, jsonrow);
	    }
	  else
	    {
	      rc = SQON_PKNOTUNIQUE;
	    }
	  json_decref (jsonrow);
	  if (rc)
	    break;
	}
      break;

    default:
      rc = SQON_UNSUPPORTED;
      break;
    }

  if (rc)
    {
      json_decref (root);
      return rc;
    }

  *out = json_dumps (root, JSON_PRESERVE_ORDER);
  json_decref (root);
  if (NULL == *out)
    {
      rc = SQON_MEMORYERROR;
    }

  return rc;
}

static int
escape (sqon_dbsrv * srv, const char *in, char *out, size_t n, bool quote)
{
  int rc = 0;
  bool connected = srv->isopen;
  size_t extra = 1 + quote ? 2 : 0;
  char *temp = calloc ((strlen (in) * 2 + extra), sizeof (char));
  if (NULL == temp)
    return SQON_MEMORYERROR;

  if (!connected)
    {
      rc = sqon_connect (srv);
      if (rc)
	{
	  free (temp);
	  return SQON_CONNECTERR;
	}
    }

  union
  {
    unsigned long ul;
  } written;

  switch (srv->type)
    {
    case SQON_DBCONN_MYSQL:
      written.ul = mysql_real_escape_string (srv->com, temp, in, strlen (in));
      if (written.ul < n)
	if (quote)
	  {
	    rc = snprintf (out, n, "'%s'", temp);
	    if ((size_t) rc >= n)
	      rc = SQON_OVERFLOW;
	    else
	      rc = 0;
	  }
	else
	  {
	    strcpy (out, temp);
	  }
      else
	rc = SQON_OVERFLOW;
      break;

    default:
      rc = SQON_UNSUPPORTED;
      break;
    }

  if (!connected)
    sqon_close (srv);

  free (temp);
  return rc;
}

int
json_to_csv (sqon_dbsrv * srv, json_t * in, char *out, size_t n,
	     bool escape_strings, bool quote)
{
  json_incref (in);
  int rc;
  json_t *value;
  size_t index, written = 1, left;
  char *temp;

  if (written >= n)
    {
      json_decref (in);
      return SQON_OVERFLOW;
    }

  if (!json_is_array (in))
    {
      json_decref (in);
      return SQON_TYPEERROR;
    }

  temp = calloc (n, sizeof (char));
  if (NULL == temp)
    {
      json_decref (in);
      return SQON_MEMORYERROR;
    }

  const char *c = ",";
  size_t clen = strlen (c);
  *out = '\0';			/* prevent appending */

  left = json_array_size (in);
  json_array_foreach (in, index, value)
  {
    switch (json_typeof (value))
      {
      case JSON_STRING:
	if (escape_strings)
	  {
	    rc = escape (srv, json_string_value (value), temp, n, quote);
	  }
	else
	  {
	    strcpy (temp, json_string_value (value));
	  }
	break;

      case JSON_INTEGER:
	rc = snprintf (temp, n, "%" JSON_INTEGER_FORMAT,
		       json_integer_value (value));
	if ((size_t) rc >= n)
	  rc = SQON_OVERFLOW;
	else
	  rc = 0;
	break;

      case JSON_REAL:
	rc = snprintf (temp, n, "%f", json_real_value (value));
	if ((size_t) rc >= n)
	  rc = SQON_OVERFLOW;
	else
	  rc = 0;
	break;

      case JSON_TRUE:
	rc = snprintf (temp, n, "1");
	if ((size_t) rc >= n)
	  rc = SQON_OVERFLOW;
	else
	  rc = 0;
	break;

      case JSON_FALSE:
	rc = snprintf (temp, n, "0");
	if ((size_t) rc >= n)
	  rc = SQON_OVERFLOW;
	else
	  rc = 0;
	break;

      case JSON_NULL:
	rc = snprintf (temp, n, "'NULL'");
	if ((size_t) rc >= n)
	  rc = SQON_OVERFLOW;
	else
	  rc = 0;
	break;

      default:
	rc = SQON_UNSUPPORTED;
	break;
      }

    if (rc)
      break;

    written += strlen (temp);

    if (--left > 0)
      {
	if (written + clen < n)
	  {
	    strcat (temp, c);
	    written += clen;
	  }
	else
	  {
	    break;
	  }
      }

    strcat (out, temp);
  }

  free (temp);
  json_decref (in);
  return rc;
}

static int
insert (sqon_dbsrv * srv, const char *table, json_t * in, char *out, size_t n)
{
  json_incref (in);
  int rc = 0;
  const char *fmt = "INSERT INTO %s(%s) VALUES(%s)";
  json_t *value, *cols, *vals;
  char *columns, *values;
  size_t index;

  if (!json_is_array (in))
    {
      json_decref (in);
      return SQON_TYPEERROR;
    }

  columns = calloc (n, sizeof (char));
  if (NULL == columns)
    {
      json_decref (in);
      return SQON_MEMORYERROR;
    }

  values = calloc (n, sizeof (char));
  if (NULL == values)
    {
      json_decref (in);
      free (columns);
      return SQON_MEMORYERROR;
    }

  json_array_foreach (in, index, value)
  {
    switch (json_typeof (value))
      {
      case JSON_OBJECT:
	cols = json_array ();
	if (NULL == cols)
	  {
	    rc = SQON_MEMORYERROR;
	    break;
	  }
	vals = json_array ();
	if (NULL == vals)
	  {
	    json_decref (cols);
	    rc = SQON_MEMORYERROR;
	    break;
	  }
	
	const char *objkey;
	json_t *objval;
	json_object_foreach (value, objkey, objval)
	  {
	    rc = json_array_append_new (cols, json_string (objkey));
	    if (rc)
	      {
		rc = SQON_MEMORYERROR;
		break;
	      }

	    rc = json_array_append (vals, objval);
	    if (rc)
	      {
		rc = SQON_MEMORYERROR;
		break;
	      }
	  }

	if (!rc)
	  {
	    rc = json_to_csv (srv, cols, columns, n, true, false);
	    if (rc)
	      break;
	    rc = json_to_csv (srv, vals, values, n, true, true);
	    if (rc)
	      break;
	  }
	json_decref (cols);
	json_decref (vals);
	break;

      default:
	rc = SQON_UNSUPPORTED;
	break;
      }

    if (rc)
      break;
  }

  if (strlen (table) > 0)
    {
      rc = snprintf (out, n, fmt, table, columns, values);
      if ((size_t) rc >= n)
	rc = SQON_OVERFLOW;
      else
	rc = 0;
    }
  else
    {
      rc = SQON_INCOMPLETE;
    }
  free (columns);
  free (values);
  json_decref (in);
  return rc;
}

int
sqon_to_sql (sqon_dbsrv * srv, const char *in, char *out, size_t n)
{
  int rc = 0;
  json_t *root, *value, *subvalue;
  char *temp;
  size_t written = 1, towrite;
  const char *key, *subkey;
  const char *semi = ";";
  const size_t slen = strlen (semi);
/*
    const char *UPDATE = "UPDATE %s SET %s %s";
    const char *SELECT = "SELECT %s %s %s";
    const char *CALL   = "CALL %s(%s)";
    const char *PERM   = "%s %s ON %s %s '%s'@'%s' %s";
*/

  if (n <= written)
    return SQON_OVERFLOW;

  root = json_loads (in, JSON_REJECT_DUPLICATES, NULL);
  if (!root)
    {
      return SQON_LOADERROR;
    }

  temp = calloc (n, sizeof (char));
  if (NULL == temp)
    {
      json_decref (root);
      return SQON_MEMORYERROR;
    }

  *out = '\0'; /* prevent appending to existing string in buffer */

  json_object_foreach (root, key, value)
  {
    if (!strcmp (key, "insert"))
      {
	if (json_is_object (value))
	  {
	    json_object_foreach (value, subkey, subvalue)
	    {
	      rc = insert (srv, subkey, subvalue, temp, n);
	      if (rc)
		break;
	      if ((towrite = written + strlen (temp)) < n)
		{
		  if ('\0' != *out)
		    if ((towrite += slen) < n)
		      {
			strcat (out, semi);
			written += slen;
		      }
		  strcat (out, temp);
		  written += strlen (temp);
		}
	      else
		{
		  rc = SQON_OVERFLOW;
		  break;
		}
	    }

	    if (rc)
	      break;
	  }
	else
	  {
	    rc = SQON_TYPEERROR;
	    break;
	  }
      }
    else if (!strcmp (key, "update"))
      {
      }
    else if (!strcmp (key, "select"))
      {
      }
    else if (!strcmp (key, "call"))
      {
      }
    else if (!strcmp (key, "grant") || !strcmp (key, "revoke"))
      {
      }
    else
      {
	rc = SQON_UNSUPPORTED;
	break;
      }
  }

  free (temp);
  json_decref (root);
  return rc;
}
