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

#define VALUE_UNIT    1
#define VALUE_CSV     2
#define VALUE_BETWEEN 3

static int
write_condition (const char *col, const char *split, const char *val,
		 char *temp, char *out, size_t n)
{
  size_t written = strlen (out);
  int to_write;

  if (written >= n)
    return SQON_OVERFLOW;

  to_write = snprintf (temp, n, "%s%s%s", col, split, val);

  written += to_write;
  if ((size_t) to_write >= n || written >= n)
    return SQON_OVERFLOW;

  strcat (out, temp);
  return 0;
}

static int
separate_condition (const char *sep, const char *buffer, char *out, size_t n)
{
  size_t blen = strlen (buffer), seplen = strlen (sep);
  size_t written = strlen (out) + (2 * blen) + seplen;

  if (written >= n)
    return SQON_OVERFLOW;

  strcat (out, buffer);
  strcat (out, sep);
  strcat (out, buffer);

  return 0;
}

static int
abstract_condition (sqon_dbsrv *srv, json_t *in, char *out, size_t n,
		    const char *sep, bool space, const char *split,
		    uint8_t val_type)
{
  int rc = 0;
  const char *key, *buffer = space ? sp : "";
  json_t *value;
  size_t left;
  char *col, *val, *temp;

  if (!json_is_object (in))
    return SQON_TYPEERROR;

  col = sqon_malloc (n * sizeof (char));
  if (NULL == col)
    return SQON_MEMORYERROR;

  val = sqon_malloc (n * sizeof (char));
  if (NULL == val)
    {
      sqon_free (col);
      return SQON_MEMORYERROR;
    }

  temp = sqon_malloc (n * sizeof (char));
  if (NULL == temp)
    {
      sqon_free (val);
      sqon_free (col);
      return SQON_MEMORYERROR;
    }

  *out = '\0';
  left = json_object_size (in);

  json_object_foreach (in, key, value)
    {
      rc = escape (srv, key, col, n, false);
      if (rc)
	break;

      switch (val_type)
	{
	case VALUE_UNIT:
	  rc = json_to_sql_type (srv, value, val, n, true);
	  break;

	case VALUE_CSV:
	  rc = json_to_csv (srv, value, val, n, true);
	  break;

	default:
	  rc = SQON_UNSUPPORTED;
	  break;
	}
      if (rc)
	break;

      rc = write_condition (col, split, val, temp, out, n);
      if (rc)
	break;

      if (--left > 0)
	rc = separate_condition (sep, buffer, out, n);
      if (rc)
	break;
    }

  sqon_free (temp);
  sqon_free (val);
  sqon_free (col);
  return rc;
}

int
equal (sqon_dbsrv *srv, json_t *in, char *out, size_t n, const char *sep,
       bool space)
{
  return abstract_condition (srv, in, out, n, sep, space, "=", VALUE_UNIT);
}

static int
in_cond (sqon_dbsrv *srv, json_t *in, char *out, size_t n, const char *sep,
	 bool space)
{
  return abstract_condition (srv, in, out, n, sep, space, " IN ", VALUE_CSV);
}

static int
like (sqon_dbsrv *srv, json_t *in, char *out, size_t n, const char *sep,
      bool space)
{
  return abstract_condition (srv, in, out, n, sep, space, " LIKE ",
			     VALUE_UNIT);
}

static int
regexp (sqon_dbsrv *srv, json_t *in, char *out, size_t n, const char *sep,
	bool space)
{
  return abstract_condition (srv, in, out, n, sep, space, " REGEXP ",
			     VALUE_UNIT);
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
	      rc = in_cond (srv, value, temp, n, sep, true);
	    }
	  else if (!strcmp (key, "like"))
	    {
	      rc = like (srv, value, temp, n, sep, true);
	    }
	  else if (!strcmp (key, "regexp"))
	    {
	      rc = regexp (srv, value, temp, n, sep, true);
	    }
	  else if (!strcmp (key, "between"))
	    {
	      rc = SQON_UNSUPPORTED;
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
