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
#include <postgresql/libpq-fe.h>
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

static void *
stored_length_malloc (size_t n)
{
  /* Store the memory area size in the beginning of the block */
  void *v = malloc (n + sizeof (size_t));

  if (NULL == v)
    return NULL;

  *((size_t *) v) = n;
  return v + sizeof (size_t);
}

static void
stored_length_free (void *v)
{
  size_t n;

  v -= sizeof (size_t);
  n = *((size_t *) v);

  safe_memset (v, 0xDF, n + sizeof (size_t));
  free (v);
}

static void *(*used_malloc) (size_t n) = stored_length_malloc;
static void (*used_free) (void *v) = stored_length_free;

void *
sqon_malloc (size_t n)
{
  return (*used_malloc) (n);
}

void
sqon_free (void *v)
{
  (*used_free) (v);
}

void
sqon_init (void)
{
  json_set_alloc_funcs (sqon_malloc, sqon_free);
}

void
sqon_set_alloc_funcs (void *(*new_malloc) (size_t n),
		      void (*new_free) (void *v))
{
  used_malloc = new_malloc;
  used_free = new_free;
}

static void *
mkbuf (const char *s)
{
  return sqon_malloc ((strlen (s) + 1) * sizeof (char));
}

sqon_DatabaseServer *
sqon_new_connection (enum sqon_database_type type, const char *host,
		     const char *user, const char *passwd,
		     const char *database, const char *port)
{
  char *thost = mkbuf (host);
  if (NULL == thost)
    return NULL;

  char *tuser = mkbuf (user);
  if (NULL == tuser)
    {
      sqon_free (thost);
      return NULL;
    }

  char *tpasswd = mkbuf (passwd);
  if (NULL == tpasswd)
    {
      sqon_free (thost);
      sqon_free (tuser);
      return NULL;
    }

  char *tdb;
  if (NULL != database)
    {
      tdb = mkbuf (database);
      if (NULL == tdb)
	{
	  sqon_free (thost);
	  sqon_free (tuser);
	  sqon_free (tpasswd);
	  return NULL;
	}
    }
  else
    {
      tdb = NULL;
    }

  char *tport = mkbuf (port);
  if (NULL == tport)
    {
      sqon_free (thost);
      sqon_free (tuser);
      sqon_free (tpasswd);
      if (tdb)
	sqon_free (tdb);
      return NULL;
    }

  sqon_DatabaseServer *out = sqon_malloc (sizeof (sqon_DatabaseServer));
  if (NULL == out)
    {
      sqon_free (thost);
      sqon_free (tuser);
      sqon_free (tpasswd);
      if (tdb)
	sqon_free (tdb);
      sqon_free (tport);
      return NULL;
    }

  strcpy (thost, host);
  strcpy (tuser, user);
  strcpy (tpasswd, passwd);
  if (tdb)
    strcpy (tdb, database);
  strcpy (tport, port);

  out->connections = 0;
  out->type = type;
  out->host = thost;
  out->user = tuser;
  out->passwd = tpasswd;
  out->database = tdb;
  out->port = tport;

  return out;
}

void
sqon_free_connection (sqon_DatabaseServer *srv)
{
  while (srv->connections)
    sqon_close (srv);

  sqon_free (srv->host);
  sqon_free (srv->user);
  sqon_free (srv->passwd);
  if (srv->database)
    sqon_free (srv->database);
  sqon_free (srv->port);
  sqon_free (srv);
}

int
sqon_connect (sqon_DatabaseServer *srv)
{
  int rc = 0;

  if (++(srv->connections) == 1)
    switch (srv->type)
      {
      case SQON_DBCONN_MYSQL:
	srv->com = mysql_init (NULL);

	const char *real_end = srv->port + strlen (srv->port);
	char *end;
	unsigned long port = strtoul (srv->port, &end, 10);
	if (end != real_end)
	  {
	    rc = SQON_CONNECTERR;
	    break;
	  }

	if (NULL == mysql_real_connect (srv->com, srv->host, srv->user,
					srv->passwd, srv->database, port, NULL,
					CLIENT_MULTI_STATEMENTS))
	  {
	    rc = (int) mysql_errno (srv->com);
	  }
	break;

      case SQON_DBCONN_POSTGRES:
	srv->com = PQsetdbLogin (srv->host, srv->port, "", "", srv->database,
				 srv->user, srv->passwd);

	if ((rc = PQstatus (srv->com)) == CONNECTION_OK)
	  rc = 0;
	break;

      default:
	rc = SQON_UNSUPPORTED;
	--srv->connections;
	break;
      }

  if (rc && rc != SQON_UNSUPPORTED)
    sqon_close (srv);

  return rc;
}

void
sqon_close (sqon_DatabaseServer *srv)
{
  if (--(srv->connections) == 0)
    switch (srv->type)
      {
      case SQON_DBCONN_MYSQL:
	mysql_close (srv->com);
	mysql_library_end ();
	break;

      case SQON_DBCONN_POSTGRES:
	PQfinish (srv->com);
	break;
      }
}

int
sqon_query (sqon_DatabaseServer *srv, const char *query, char **out,
	    const char *pk)
{
  int rc;
  union res res;

  res.mysql = NULL;

  rc = sqon_connect (srv);
  if (rc)
    return rc;

  switch (srv->type)
    {
    case SQON_DBCONN_MYSQL:
      if (mysql_query (srv->com, query))
	{
	  rc = mysql_errno (srv->com);
	  sqon_close (srv);
	  return rc;
	}
      break;

    case SQON_DBCONN_POSTGRES:
      res.postgres = PQexec (srv->com, query);
      if ((rc = PQresultStatus (res.postgres)) != PGRES_COMMAND_OK)
	{
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

	case SQON_DBCONN_POSTGRES:
	  rc = res_to_json (SQON_DBCONN_POSTGRES, res.postgres, out, pk);
	  PQclear (res.postgres);
	  break;
	}
    }
  else
    {
      sqon_close (srv);
    }

  switch (srv->type)
    {
    case SQON_DBCONN_POSTGRES:
      PQclear (res.postgres);
      break;
    }

  return rc;
}

int
sqon_get_primary_key (sqon_DatabaseServer *srv, const char *table, char **out)
{
  int rc;
  char *query, *esc_table;
  size_t qlen = 1;
  const char *fmt, *key_column;
  union res res;
  union row row;

  switch (srv->type)
    {
    case SQON_DBCONN_MYSQL:
      fmt = "SHOW KEYS FROM %s WHERE Key_name = 'PRIMARY'";
      key_column = "Column_name";
      break;

    case SQON_DBCONN_POSTGRES:
      fmt = "SELECT c.column_name FROM information_schema.table_constraints tc"
	" JOIN information_schema.constraint_column_usage AS ccu"
	  " USING (constraint_schema, constraing_name)"
	" JOIN information_schema.columns AS c"
	  " ON c.table_schema = tc.constraint_schema"
	    " AND tc.table_name = c.table_name"
	    " AND ccu.column_name = c.column_name"
	" WHERE constraint_type = 'PRIMARY KEY' AND tc.table_name = '%s'";
      key_column = "column_name";
      break;

    default:
      return SQON_UNSUPPORTED;
    }

  esc_table = sqon_malloc ((strlen (table) * 2 + 1) * sizeof (char));
  if (NULL == esc_table)
    return SQON_MEMORYERROR;

  rc = sqon_escape (srv, table, &esc_table, false);
  if (rc)
    {
      sqon_free (esc_table);
      return rc;
    }

  qlen += strlen (fmt);
  qlen += strlen (esc_table);

  query = sqon_malloc (qlen * sizeof (char));
  if (NULL == query)
    {
      sqon_free (esc_table);
      return SQON_MEMORYERROR;
    }

  rc = snprintf (query, qlen, fmt, esc_table);
  sqon_free (esc_table);
  if ((size_t) rc >= qlen)
    {
      sqon_free (query);
      return SQON_OVERFLOW;
    }

  rc = sqon_connect (srv);
  if (rc)
    {
      sqon_free (query);
      return rc;
    }

  char *res;
  rc = sqon_query (srv, query, &res, NULL);
  sqon_free (query);
  if (rc)
    return rc;

  json_t *res_set = json_loads (res, JSON_ALLOW_NUL, NULL);
  sqon_free (res);
  if (NULL == res_set)
    return SQON_MEMORYERROR;

  json_t *res_obj = json_array_get (res_set, 0);
  if (NULL == res_obj)
    {
      json_decref (res_set);
      return SQON_NOPK;
    }

  const char *primary_key = json_string_value (json_object_get (res_obj,
								key_column));

  *out = sqon_malloc ((strlen (primary_key) + 1) * sizeof (char));
  if (NULL == *out)
    {
      json_decref (res_set);
      return SQON_MEMORYERROR;
    }

  strcpy (*out, primary_key);
  json_decref (res_set);

  return 0;
}

int
sqon_escape (sqon_DatabaseServer *srv, const char *in, char **out, bool quote)
{
  int rc;
  size_t extra = 1 + (quote ? 2 : 0);
  char *temp, *quoted;
  union
  {
    unsigned long ul;
  } written;

  temp = sqon_malloc ((strlen (in) * 2 + extra) * sizeof (char));
  if (NULL == temp)
    return SQON_MEMORYERROR;

  rc = sqon_connect (srv);
  if (rc)
    {
      sqon_free (temp);
      return rc;
    }

  switch (srv->type)
    {
    case SQON_DBCONN_MYSQL:
      written.ul = mysql_real_escape_string (srv->com, temp, in, strlen (in));
      written.ul += quote ? 3 : 1;

      *out = sqon_malloc (written.ul * sizeof (char));
      if (NULL == *out)
	{
	  rc = SQON_MEMORYERROR;
	  break;
	}

      if (quote)
	snprintf (*out, written.ul, "'%s'", temp);
      else
	strcpy (*out, temp);
      break;

    case SQON_DBCONN_POSTGRES:
      quoted = PQescapeLiteral (srv->com, in, strlen (in));
      if (NULL == quoted)
	{
	  rc = SQON_MEMORYERROR;
	  break;
	}

      if (quote)
	{
	  strcpy (temp, quoted);
	}
      else
	{
	  quoted[strlen (quoted) - 1] = '\0';
	  strcpy (temp, quoted + 1);
	}
      PQfreemem (quoted);

      *out = sqon_malloc ((strlen (temp) + 1) * sizeof (char));
      if (NULL == *out)
	{
	  rc = SQON_MEMORYERROR;
	  break;
	}

      strcpy (*out, temp);
      break;

    default:
      rc = SQON_UNSUPPORTED;
      break;
    }

  sqon_close (srv);

  sqon_free (temp);
  return rc;
}
