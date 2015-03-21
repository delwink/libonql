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

/**
 * @file sqon.h
 * @version 0.0
 * @date 02/14/2015
 * @author David McMackins II
 * @brief C implementation for Delwink's SQON
 */

#ifndef DELWINK_SQON_H
#define DELWINK_SQON_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/**
 * @brief libsqon software version
 */
#define SQON_VERSION "0.0.3"

/**
 * @brief Information about the libsqon copyright holders and license.
 */
#define SQON_COPYRIGHT \
"libsqon - C API for Delwink's Structured Query Object Notation\n"\
"Copyright (C) 2015 Delwink, LLC\n\n"\
"This program is free software: you can redistribute it and/or modify\n"\
"it under the terms of the GNU Affero General Public License as published by\n"\
"the Free Software Foundation, version 3 only.\n\n"\
"This program is distributed in the hope that it will be useful,\n"\
"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"\
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"\
"GNU Affero General Public License for more details.\n\n"\
"You should have received a copy of the GNU Affero General Public License\n"\
"along with this program.  If not, see <http://www.gnu.org/licenses/>."

/**
 * @brief Error allocating memory.
 */
#define SQON_MEMORYERROR -12

/**
 * @brief Buffer overflow.
 */
#define SQON_OVERFLOW    -13

/**
 * @brief Unsupported SQON object.
 */
#define SQON_UNSUPPORTED -14

/**
 * @brief Error connecting to database server.
 */
#define SQON_CONNECTERR  -20

/**
 * @brief Expecting return columns, but found none.
 */
#define SQON_NOCOLUMNS   -21

/**
 * @brief Specified primary key was not returned.
 */
#define SQON_NOPK        -23

/**
 * @brief Specified primary key was not unique among returned data.
 */
#define SQON_PKNOTUNIQUE -24

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief More secure implementation of stdlib's malloc().
 * @param n Number of bytes to allocate on the heap.
 * @return Pointer to n bytes of available memory.
 */
void
*sqon_malloc (size_t n);

/**
 * @brief Frees memory allocated with sqon_malloc().
 * @param v Pointer returned by earlier call to sqon_malloc().
 */
void
sqon_free (void *v);

/**
 * @brief Initializes SQON and supporting libraries.
 */
void
sqon_init (void);

/**
 * @brief The universal database connection auxiliary structure for libsqon.
 */
typedef struct dbconn
{
  void *com;
  bool isopen;
  uint8_t type;
  const char *host;
  const char *user;
  const char *passwd;
  const char *database;
} sqon_dbsrv;

/**
 * @brief Connection type constant for MySQL-based databases.
 */
#define SQON_DBCONN_MYSQL 1

/**
 * @brief Constructs a database connection.
 * @param type Connection type constant, such as SQON_DBCONN_MYSQL.
 * @param host The hostname or IP address of the database server.
 * @param user The username with which to authenticate with the database server.
 * @param passwd The password by which to be authenticated.
 * @param database The name of the database to be used; can be NULL.
 * @return A new database connection object.
 */
sqon_dbsrv
sqon_new_connection (uint8_t type, const char *host, const char *user,
		     const char *passwd, const char *database);

/**
 * @brief Connects to the database server.
 * @param srv Initialized database connection object.
 * @return Negative if input error; positive if connection error.
 */
int
sqon_connect (sqon_dbsrv *srv);

/**
 * @brief Closes connection to the database server.
 * @param srv Connected database connection object.
 */
void
sqon_close (sqon_dbsrv *srv);

/**
 * @brief Query the database.
 * @param srv Initialized database connection object.
 * @param query UTF-8 encoded SQL statement.
 * @param out Pointer to string which will be allocated and populated with
 * JSON-formatted response from the database; must free with sqon_free();
 * populated as object if table has a primary key, else an array; can be NULL
 * if no result is expected.
 * @param pk Primary key expected in return value, if any (else NULL).
 * @return Negative if input or IO error; positive if error from server.
 */
int
sqon_query (sqon_dbsrv *srv, const char *query, char **out, const char *pk);

/**
 * @brief Gets the primary key of a table.
 * @param srv Initialized database connection object.
 * @param table Name of the database table.
 * @param out Pointer to string which will be allocated and populated with the
 * table's primary key.
 * @return Negative if input or IO error; positive if error from server.
 */
int
sqon_get_pk (sqon_dbsrv *srv, const char *table, char **out);

/**
 * @brief Properly escapes a string for the database engine.
 * @param srv Initialized database connection object.
 * @param in String to be escaped.
 * @param out Output buffer for the escaped string.
 * @param n Size of the output buffer.
 * @param quote Whether to surround the output string in apostrophes.
 * @return Nonzero on error.
 */
int
sqon_escape (sqon_dbsrv *srv, const char *in, char *out, size_t n, bool quote);

#ifdef __cplusplus
}
#endif

#endif
