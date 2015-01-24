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

#include "sqlquery.h"
#include "sqlstatement.h"
#include "sqlcondition.h"

#define PERM_GRANT 1
#define PERM_REVOKE 2

#define semi ";"
#define slen 1

static int
check_pk (uint8_t type, union fields fields, size_t num_fields, const char *pk)
{
  int rc = 0;
  size_t i;

  switch (type)
    {
    case SQON_DBCONN_MYSQL:
      for (i = 0; i < num_fields; ++i)
	{
	  if (strcmp (fields.mysql[i].name, pk))
	    rc = SQON_NOPK;
	  else
	    break;
	}
      break;

    default:
      rc = SQON_UNSUPPORTED;
      break;
    }

  return rc;
}

static int
make_json_row (uint8_t type, union fields fields, union row row, json_t *root,
	       bool arr, size_t num_fields, const char *pk)
{
  int rc = 0;
  size_t i;
  char *vpk;
  json_t *jsonrow = json_object ();

  if (NULL == jsonrow)
    return SQON_MEMORYERROR;

  switch (type)
    {
    case SQON_DBCONN_MYSQL:
      for (i = 0; i < num_fields; ++i)
	{
	  if (arr || strcmp (fields.mysql[i].name, pk))
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
      break;

    default:
      rc = SQON_UNSUPPORTED;
      break;
    }

  if (!rc)
    {
      if (arr)
	json_array_append (root, jsonrow);
      else if (NULL == json_object_get (root, vpk))
	json_object_set (root, vpk, jsonrow);
      else
	rc = SQON_PKNOTUNIQUE;
    }

  json_decref (jsonrow);
  return rc;
}

int
res_to_json (uint8_t type, void *res, char **out, const char *pk)
{
  int rc = 0;
  bool arr = (NULL == pk || !strcmp (pk, ""));
  json_t *root;
  size_t num_fields;
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
	  rc = check_pk (type, fields, num_fields, pk);
	  if (rc)
	    break;
	}

      while ((row.mysql = mysql_fetch_row (res)))
	{
	  rc = make_json_row (type, fields, row, root, arr, num_fields, pk);
	  if (rc)
	    break;
	}
      break;

    default:
      rc = SQON_UNSUPPORTED;
      break;
    }

  if (!rc)
    {
      *out = json_dumps (root, JSON_PRESERVE_ORDER);
      if (NULL == *out)
	rc = SQON_MEMORYERROR;
    }

  json_decref (root);
  return rc;
}

int
escape (sqon_dbsrv *srv, const char *in, char *out, size_t n, bool quote)
{
  int rc = 0;
  bool connected = srv->isopen;
  size_t extra = 1 + quote ? 2 : 0;
  char *temp = sqon_malloc ((strlen (in) * 2 + extra) * sizeof (char));
  if (NULL == temp)
    return SQON_MEMORYERROR;

  if (!connected)
    {
      rc = sqon_connect (srv);
      if (rc)
	{
	  sqon_free (temp);
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
      if (written.ul >= n)
	{
	  rc = SQON_OVERFLOW;
	  break;
	}

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
      break;

    default:
      rc = SQON_UNSUPPORTED;
      break;
    }

  if (!connected)
    sqon_close (srv);

  sqon_free (temp);
  return rc;
}

int
json_to_sql_type (sqon_dbsrv *srv, json_t *in, char *out, size_t n, bool quote)
{
  json_incref (in);
  int rc = 0;
  const char *s;
  
  switch (json_typeof (in))
    {
    case JSON_STRING:
      s = json_string_value (in);
      bool stillquote = quote;
      if (quote && !(stillquote = '\\' != *s))
	++s;
      rc = escape (srv, s, out, n, stillquote);
      break;
      
    case JSON_INTEGER:
      rc = snprintf (out, n, "%" JSON_INTEGER_FORMAT,
		     json_integer_value (in));
      if ((size_t) rc >= n)
	rc = SQON_OVERFLOW;
      else
	rc = 0;
      break;
      
    case JSON_REAL:
      rc = snprintf (out, n, "%f", json_real_value (in));
      if ((size_t) rc >= n)
	rc = SQON_OVERFLOW;
      else
	rc = 0;
      break;
      
    case JSON_TRUE:
      rc = snprintf (out, n, "1");
      if ((size_t) rc >= n)
	rc = SQON_OVERFLOW;
      else
	rc = 0;
      break;
      
    case JSON_FALSE:
      rc = snprintf (out, n, "0");
      if ((size_t) rc >= n)
	rc = SQON_OVERFLOW;
      else
	rc = 0;
      break;
      
    case JSON_NULL:
      rc = snprintf (out, n, "'NULL'");
      if ((size_t) rc >= n)
	rc = SQON_OVERFLOW;
      else
	rc = 0;
      break;
      
    default:
      rc = SQON_UNSUPPORTED;
      break;
    }

  json_decref (in);
  return rc;
}

static int
write_query_string (size_t *written, const char *temp, char *out, size_t n)
{
  size_t towrite = *written + strlen (temp);

  if (towrite < n)
    {
      if ('\0' != *out)
	{
	  if ((towrite += slen) < n)
	    {
	      strcat (out, semi);
	      *written += slen;
	    }
	  else
	    {
	      return SQON_OVERFLOW;
	    }
	}

      strcat (out, temp);
      *written += strlen (temp);
    }
  else
    {
      return SQON_OVERFLOW;
    }

  return 0;
}

int
sqon_to_sql (sqon_dbsrv * srv, const char *in, char *out, size_t n)
{
  int rc = 0;
  json_t *root, *value, *subvalue;
  char *temp;
  size_t written = 1;
  const char *key, *subkey;
/*
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

  temp = sqon_malloc (n * sizeof (char));
  if (NULL == temp)
    {
      json_decref (root);
      return SQON_MEMORYERROR;
    }

  *out = '\0'; /* prevent appending to existing string in buffer */

  json_object_foreach (root, key, value)
    {
      if (!json_is_object (value))
	{
	  rc = SQON_TYPEERROR;
	  break;
	}

      if (!strcmp (key, "insert"))
	{
	  json_object_foreach (value, subkey, subvalue)
	    {
	      rc = insert (srv, subkey, subvalue, temp, n);
	      if (rc)
		break;
	      rc = write_query_string (&written, temp, out, n);
	      if (rc)
		break;
	    }
	  
	  if (rc)
	    break;
	}
      else if (!strcmp (key, "update"))
	{
	  json_object_foreach (value, subkey, subvalue)
	    {
	      rc = update (srv, subkey, subvalue, temp, n);
	      if (rc)
		break;
	      rc = write_query_string (&written, temp, out, n);
	      if (rc)
		break;
	    }

	  if (rc)
	    break;
	}
      else if (!strcmp (key, "select"))
	{
	  rc = select (srv, value, temp, n);
	  if (rc)
	    break;
	  rc = write_query_string (&written, temp, out, n);
	  if (rc)
	    break;
	}
      else if (!strcmp (key, "call"))
	{
	}
      else if (!strcmp (key, "grant") || !strcmp (key, "revoke"))
	{
	}
      else if (!strcmp (key, "delete"))
	{
	}
      else if (!strcmp (key, "show"))
	{
	}
      else
	{
	  rc = SQON_UNSUPPORTED;
	  break;
	}
    }

  sqon_free (temp);
  json_decref (root);
  return rc;
}
