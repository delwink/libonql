/*
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

#include <jansson.h>
#include <string.h>

#include "sqlstatement.h"
#include "sqlquery.h"
#include "sqlcondition.h"

static int
json_to_csv (sqon_dbsrv *srv, json_t *in, char *out, size_t n, bool quote)
{
  json_incref (in);
  int rc = 0;
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

  temp = sqon_malloc (n * sizeof (char));
  if (NULL == temp)
    {
      json_decref (in);
      return SQON_MEMORYERROR;
    }

  *out = '\0';

  left = json_array_size (in);
  json_array_foreach (in, index, value)
    {
      json_to_sql_type(srv, value, temp, n, quote);
      
      if (rc)
	break;
      
      written += strlen (temp);
      
      if (--left > 0)
	{
	  if ((written += clen) < n)
	    {
	      strcat (temp, c);
	    }
	  else
	    {
	      rc = SQON_OVERFLOW;
	      break;
	    }
	}
      
      strcat (out, temp);
    }
  
  sqon_free (temp);
  json_decref (in);
  return rc;
}

static int
insert_cols_vals (sqon_dbsrv *srv, json_t *in, char *columns, char *values,
		  size_t n)
{
  int rc = 0;
  json_t *cols, *vals, *value;
  const char *key;

  switch (json_typeof (in))
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

      json_object_foreach (in, key, value)
	{
	  rc = json_array_append_new (cols, json_string (key));
	  if (rc)
	    {
	      rc = SQON_MEMORYERROR;
	      break;
	    }

	  rc = json_array_append (vals, value);
	  if (rc)
	    {
	      rc = SQON_MEMORYERROR;
	      break;
	    }
	}

      if (!rc)
	{
	  rc = json_to_csv (srv, cols, columns, n, false);
	  if (rc)
	    {
	      json_decref (cols);
	      json_decref (vals);
	      break;
	    }
	  rc = json_to_csv (srv, vals, values, n, true);
	}

      json_decref (cols);
      json_decref (vals);
      break;

    default:
      rc = SQON_UNSUPPORTED;
      break;
    }

  return rc;
}

int
insert (sqon_dbsrv *srv, const char *table, json_t *in, char *out, size_t n)
{
  json_incref (in);
  int rc = 0;
  const char *fmt = "INSERT INTO %s(%s) VALUES(%s)";
  json_t *value;
  char *columns, *values;
  size_t index;

  if (!json_is_array (in))
    {
      json_decref (in);
      return SQON_TYPEERROR;
    }

  if (!(strlen (table) > 0))
    {
      json_decref (in);
      return SQON_INCOMPLETE;
    }

  columns = sqon_malloc (n * sizeof (char));
  if (NULL == columns)
    {
      json_decref (in);
      return SQON_MEMORYERROR;
    }

  values = sqon_malloc (n * sizeof (char));
  if (NULL == values)
    {
      json_decref (in);
      sqon_free (columns);
      return SQON_MEMORYERROR;
    }

  json_array_foreach (in, index, value)
    {
      rc = insert_cols_vals (srv, value, columns, values, n);

      if (rc)
	break;
    }

  rc = snprintf (out, n, fmt, table, columns, values);
  if ((size_t) rc >= n)
    rc = SQON_OVERFLOW;
  else
    rc = 0;

  sqon_free (columns);
  sqon_free (values);
  json_decref (in);
  return rc;
}

int
update (sqon_dbsrv *srv, const char *table, json_t *in, char *out, size_t n)
{
  json_incref (in);
  int rc = 0;
  const char *fmt = "UPDATE %s SET %s %s";
  json_t *value;
  char *key, *set, *conditions;

  if (!json_is_object (in))
    {
      json_decref (in);
      return SQON_TYPEERROR;
    }

  if (!(strlen (table) > 0))
    {
      json_decref (in);
      return SQON_INCOMPLETE;
    }

  set = sqon_malloc (n * sizeof (char));
  if (NULL == set)
    {
      json_decref (in);
      return SQON_MEMORYERROR;
    }

  conditions = sqon_malloc (n * sizeof (char));
  if (NULL == conditions)
    {
      json_decref (in);
      sqon_free (set);
      return SQON_MEMORYERROR;
    }

  conditions[0] = '\0';

  json_object_foreach (in, key, value)
    {
      switch (json_typeof (value))
	{
	case JSON_OBJECT:
	  if (!strcmp (key, "values"))
	    {
	      rc = equal (srv, value, set, n, c, false);
	    }
	  else if (!strcmp (key, "where"))
	    {
	      rc = sqlcondition (srv, value, conditions, n);
	    }
	  else
	    {
	      rc = SQON_UNSUPPORTED;
	    }
	  break;

	default:
	  rc = SQON_UNSUPPORTED;
	  break;
	}
    }

  if (!rc && (size_t) snprintf (out, n, fmt, table, set, conditions) >= n)
    {
      rc = SQON_OVERFLOW;
    }

  sqon_free (conditions);
  sqon_free (set);
  json_decref (in);
  return rc;
}
