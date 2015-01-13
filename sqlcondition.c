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
#include "internal.h"
#include "sqlcondition.h"

#define c ","
#define clen strlen (c)

int
equal (sqon_dbsrv *srv, json_t *in, char *out, size_t n)
{
  json_incref (in);
  int rc = 0;
  const char *key;
  size_t written = 1;
  json_t *value;
  char *col, *val, *temp;

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

  json_object_foreach (in, key, value)
    {
      rc = escape(srv, key, col, n, false);
      if (rc)
	break;
      rc = json_to_sql_type(srv, value, val, n, true);
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

      if ('\0' != *out && (written += clen) < n)
	{
	  strcat (out, c);
	}
      else
	{
	  rc = SQON_OVERFLOW;
	  break;
	}

      strcat (out, temp);
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
  const char *key;
  json_t *value;
  char *temp;
  size_t written = 1;

  if (!json_is_object (in))
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

  size_t left = json_object_size (in);
  json_object_foreach (in, key, value)
    {
      switch (json_typeof (value))
	{
	case JSON_OBJECT:
	  if (!strcmp (key, "equal"))
	    {
	      rc = equal (srv, value, temp, n);
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
	  if ((written += clen) < n)
	    strcat (out, c);
	  else
	    rc = SQON_OVERFLOW;
	}

      if (rc)
	break;
    }

  sqon_free (temp);
  json_decref (in);
  return rc;
}
