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
 * @version 1.2
 * @date 07/21/2015
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
#define SQON_VERSION "1.2.0"

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

#ifdef __cplusplus
# define __BEGIN_DECLS extern "C" {
# define __END_DECLS }
#else
# define __BEGIN_DECLS
# define __END_DECLS
#endif

__BEGIN_DECLS

/**
 * @brief Error codes returned on failure.
 */
enum sqon_error
  {
    SQON_MEMORYERROR = -12,
    SQON_OVERFLOW    = -13,
    SQON_UNSUPPORTED = -14,

    SQON_CONNECTERR  = -20,
    SQON_NOCOLUMNS   = -21,
    SQON_NOPK        = -23,
    SQON_PKNOTUNIQUE = -24
  };

/**
 * @brief More secure implementation of stdlib's malloc().
 * @param n Number of bytes to allocate on the heap.
 * @return Pointer to n bytes of available memory.
 */
void *
sqon_malloc (size_t n);

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
 * @brief Changes the memory management functions used internally.
 * @param new_malloc The new malloc() function to be used.
 * @param new_free The new free() function to be used.
 */
void
sqon_set_alloc_funcs (void *(*new_malloc) (size_t n),
		      void (*new_free) (void *v));

/**
 * @brief The universal database connection auxiliary structure for libsqon.
 */
typedef struct
{
  void *com;
  uint64_t connections;
  uint8_t type;
  char *host;
  char *user;
  char *passwd;
  char *database;
  char *port;
} sqon_DatabaseServer;

/**
 * @brief Constants for storing the type of a database connection.
 */
enum sqon_database_type
  {
    SQON_DBCONN_MYSQL = 1,
    SQON_DBCONN_POSTGRES
  };

/**
 * @brief Constructs a database connection.
 * @param type Connection type constant, such as SQON_DBCONN_MYSQL.
 * @param host The hostname or IP address of the database server.
 * @param user The username with which to authenticate with the database server.
 * @param passwd The password by which to be authenticated.
 * @param database The name of the database to be used; can be NULL.
 * @param port String representation of the port number (can be 0 for default).
 * @return A new database connection object which much be freed with
 * sqon_free_connection() or NULL on failure.
 */
sqon_DatabaseServer *
sqon_new_connection (enum sqon_database_type type, const char *host,
		     const char *user, const char *passwd,
		     const char *database, const char *port);

/**
 * @brief Destructs a database connection object.
 * @param srv Initialized database connection object to be freed.
 */
void
sqon_free_connection (sqon_DatabaseServer *srv);

/**
 * @brief Connects to the database server.
 * @param srv Initialized database connection object.
 * @return Negative if input error; positive if connection error.
 */
int
sqon_connect (sqon_DatabaseServer *srv);

/**
 * @brief Closes connection to the database server.
 * @param srv Connected database connection object.
 */
void
sqon_close (sqon_DatabaseServer *srv);

/**
 * @brief Query the database.
 * @param srv Initialized database connection object.
 * @param query UTF-8 encoded SQL statement.
 * @param out Pointer to string which will be allocated and populated with
 * JSON-formatted response from the database; must free with sqon_free();
 * populated as object if table has a primary key, else an array; can be NULL
 * if no result is expected.
 * @param primary_key Primary key expected in return value, if any (else NULL).
 * @return Negative if input or IO error; positive if error from server.
 */
int
sqon_query (sqon_DatabaseServer *srv, const char *query, char **out,
	    const char *primary_key);

/**
 * @brief Gets the primary key of a table.
 * @param srv Initialized database connection object.
 * @param table Name of the database table, not escaped.
 * @param out Pointer to string which will be allocated and populated with the
 * table's primary key.
 * @return Negative if input or IO error; positive if error from server.
 */
int
sqon_get_primary_key (sqon_DatabaseServer *srv, const char *table, char **out);

/**
 * @brief Properly escapes a string for the database engine.
 * @param srv Initialized database connection object.
 * @param in String to be escaped.
 * @param out Pointer to string which will be allocated and populated with the
 * escaped value.
 * @param quote Whether to surround the output string in apostrophes.
 * @return Nonzero on error.
 */
int
sqon_escape (sqon_DatabaseServer *srv, const char *in, char **out, bool quote);

__END_DECLS

#endif
