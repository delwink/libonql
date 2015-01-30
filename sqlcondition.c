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

#include <stdlib.h>
#include <string.h>

#include "sqon.h"
#include "sqlquery.h"
#include "sqlcondition.h"

int
equal (sqon_dbsrv *srv, json_t *in, char *out, size_t n, const char *sep,
       bool space)
{
  json_incref (in);
  int rc = 0;
  const char *key, *buffer = space ? sp : "";
  size_t written = 1, blen = strlen (buffer);
  json_t *value;
  char *col, *val, *temp;

  if (!json_is_object (in))
    {
      json_decref (in);
      return SQON_TYPEERROR;
    }

  col = sqon_malloc (n * sizeof (char));
  if (NULL == col)
    {
      json_decref (in);
      return SQON_MEMORYERROR;
    }

  val = sqon_malloc (n * sizeof (char));
  if (NULL == val)
    {
      json_decref (in);
      sqon_free (col);
      return SQON_MEMORYERROR;
    }

  temp = sqon_malloc (n * sizeof (char));
  if (NULL == temp)
    {
      json_decref (in);
      sqon_free (col);
      sqon_free (val);
      return SQON_MEMORYERROR;
    }

  *out = '\0';
  size_t seplen = strlen (sep);
  size_t left = json_object_size (in);

  json_object_foreach (in, key, value)
    {
      rc = escape (srv, key, col, n, false);
      if (rc)
	break;
      rc = json_to_sql_type (srv, value, val, n, true);
      if (rc)
	break;

      rc = snprintf (temp, n, "%s=%s", col, val);
      if ((size_t) rc >= n)
	{
	  rc = SQON_OVERFLOW;
	}
      else
	{
	  written += rc;
	  rc = 0;
	}

      if (rc)
	break;

      strcat (out, temp);

      if (--left > 0)
	{
	  written += (blen * 2) + seplen;
	  if (written < n)
	    {
	      strcat (out, buffer);
	      strcat (out, sep);
	      strcat (out, buffer);
	    }
	  else
	    {
	      rc = SQON_OVERFLOW;
	      break;
	    }
	}
    }

  sqon_free (temp);
  sqon_free (col);
  sqon_free (val);
  json_decref (in);
  return rc;
}

int
sqlcondition (sqon_dbsrv *srv, json_t *in, char *out, size_t n)
{
  json_incref (in);
  int rc = 0;
  const char *key, *sep;
  json_t *value;
  char *temp;
  size_t written = 1;

  if (!json_is_object (in))
    {
      json_decref (in);
      return SQON_TYPEERROR;
    }

  json_t *jsep = json_object_get (in, "separator");
  if (NULL == jsep)
    {
      sep = "AND";
    }
  else
    {
      sep = json_string_value (jsep);
      if (NULL == sep)
	{
	  json_decref (in);
	  return SQON_TYPEERROR;
	}
    }

  size_t seplen = strlen (sep);

  temp = sqon_malloc (n * sizeof (char));
  if (NULL == temp)
    {
      json_decref (in);
      return SQON_MEMORYERROR;
    }

  if ((size_t) snprintf (out, n, "WHERE ") >= n)
    {
      json_decref (in);
      sqon_free (temp);
      return SQON_OVERFLOW;
    }

  size_t left = json_object_size (in);
  json_object_foreach (in, key, value)
    {
      switch (json_typeof (value))
	{
	case JSON_OBJECT:
	  if (!strcmp (key, "equal"))
	    {
	      rc = equal (srv, value, temp, n, sep, true);
	    }
	  else if (!strcmp (key, "in"))
	    {
	    }
	  else if (!strcmp (key, "like"))
	    {
	    }
	  else if (!strcmp (key, "regexp"))
	    {
	    }
	  else if (!strcmp (key, "between"))
	    {
	    }
	  else
	    {
	      rc = SQON_UNSUPPORTED;
	    }
	  break;

	case JSON_STRING:
	  if (!strcmp (key, "separator"))
	    {
	      /* Handled above. */
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

      strcat (out, temp);
      written += strlen (temp);

      if (--left > 0)
	{
	  written += (splen * 2) + seplen;
	  if (written < n)
	    {
	      strcat (out, sp);
	      strcat (out, sep);
	      strcat (out, sp);
	    }
	  else
	    {
	      rc = SQON_OVERFLOW;
	      break;
	    }
	}
    }

  sqon_free (temp);
  json_decref (in);
  return rc;
}
