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

#include "result.h"

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

static json_t *
row_string (const char *str)
{
  return NULL == str ? json_null () : json_string (str);
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
				      row_string (row.mysql[i]));
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
