/*
 *  Structured Query Object Notation - C API
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

#include <jansson.h>
#include <mysql/mysql.h>
#include <string.h>

#include "sqon.h"
#include "result.h"

#define empty "[]"
#define emptylen strlen (empty)

static void *
safe_memset (void *v, int c, size_t n)
{
  volatile char *p = v;
  while (n--)
    *p++ = c;

  return v;
}

void *
sqon_malloc (size_t n)
{
  /* Store the memory area size in the beginning of the block */
  void *v = malloc (n + sizeof (size_t));

  if (NULL == v)
    return NULL;

  *((size_t *) v) = n;
  return v + sizeof (size_t);
}

void
sqon_free (void *v)
{
  size_t n;

  v -= sizeof (size_t);
  n = *((size_t *) v);

  safe_memset (v, 0, n + sizeof (size_t));
  free (v);
}

void
sqon_init (void)
{
  json_set_alloc_funcs (sqon_malloc, sqon_free);
}

struct dbconn
sqon_new_connection (uint8_t type, const char *host,
		     const char *user, const char *passwd,
		     const char *database)
{
  struct dbconn out = {
    .isopen = false,
    .type = type,
    .host = host,
    .user = user,
    .passwd = passwd,
    .database = database
  };
  return out;
}

int
sqon_connect (sqon_dbsrv *srv)
{
  switch (srv->type)
    {
    case SQON_DBCONN_MYSQL:
      srv->com = mysql_init (NULL);
      if (NULL == mysql_real_connect (srv->com, srv->host, srv->user,
				      srv->passwd, srv->database, 0, NULL,
				      CLIENT_MULTI_STATEMENTS))
	return (int) mysql_errno (srv->com);
      break;

    default:
      return SQON_CONNECTERR;
    }

  srv->isopen = true;
  return 0;
}

void
sqon_close (sqon_dbsrv *srv)
{
  switch (srv->type)
    {
    case SQON_DBCONN_MYSQL:
      mysql_close (srv->com);
      srv->isopen = false;
      break;
    }
}

int
sqon_query (sqon_dbsrv *srv, const char *query, char **out, const char *pk)
{
  int rc = 0;
  bool connected = srv->isopen;
  union res res;

  if (!connected)
    {
      rc = sqon_connect (srv);
      if (rc)
	return rc;
    }

  switch (srv->type)
    {
    case SQON_DBCONN_MYSQL:
      if (mysql_query (srv->com, query))
	{
	  rc = mysql_errno (srv->com);
	  if (!connected)
	    sqon_close (srv);
	  return rc;
	}
      break;

    default:
      return SQON_UNSUPPORTED;
    }

  if (NULL != out)
    {
      switch (srv->type)
	{
	case SQON_DBCONN_MYSQL:
	  res.mysql = mysql_store_result (srv->com);
	  if (!connected)
	    sqon_close (srv);
	  if (NULL == res.mysql)
	    {
	      rc = (int) mysql_errno (srv->com);
	      if (rc)
		return rc;
	      *out = sqon_malloc ((emptylen + 1) * sizeof (char));
	      strcpy (*out, empty);
	    }
	  else
	    {
	      rc = res_to_json (SQON_DBCONN_MYSQL, res.mysql, out, pk);
	    }
	  mysql_free_result (res.mysql);
	  break;
	}
    }

  return rc;
}

int
sqon_get_pk (sqon_dbsrv *srv, const char *table, char **out)
{
  int rc;
  bool connected = srv->isopen;
  char *query;
  size_t qlen = 1;
  const char *fmt;
  union res res;
  union row row;

  switch (srv->type)
    {
    case SQON_DBCONN_MYSQL:
      fmt = "SHOW KEYS FROM %s WHERE Key_name = 'PRIMARY'";
      break;

    default:
      return SQON_UNSUPPORTED;
    }

  qlen += strlen (fmt);
  qlen += strlen (table);

  query = sqon_malloc (qlen * sizeof (char));
  if (NULL == query)
    return SQON_MEMORYERROR;

  rc = snprintf (query, qlen, fmt, table);
  if ((size_t) rc >= qlen)
    {
      sqon_free (query);
      return SQON_OVERFLOW;
    }

  if (!connected)
    {
      rc = sqon_connect (srv);
      if (rc)
	{
	  sqon_free (query);
	  return rc;
	}
    }

  switch (srv->type)
    {
    case SQON_DBCONN_MYSQL:
      rc = mysql_query (srv->com, query);
      sqon_free (query);
      if (rc)
	{
	  if (!connected)
	    sqon_close (srv);
	  return rc;
	}

      res.mysql = mysql_store_result (srv->com);
      if (!connected)
	sqon_close (srv);

      if (NULL == res.mysql)
	return (int) mysql_errno (srv->com);

      row.mysql = mysql_fetch_row (res.mysql);
      if (!row.mysql)
	{
	  mysql_free_result (res.mysql);
	  return SQON_NOPK;
	}

      size_t len = strlen (row.mysql[4]);
      *out = sqon_malloc ((len + 1) * sizeof (char));
      if (NULL == *out)
	{
	  mysql_free_result (res.mysql);
	  return SQON_MEMORYERROR;
	}

      strcpy (*out, row.mysql[4]);
      mysql_free_result (res.mysql);
      break;
    }

  return 0;
}

int
sqon_escape (sqon_dbsrv *srv, const char *in, char *out, size_t n, bool quote)
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
