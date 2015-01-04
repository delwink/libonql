/*
 *  Structured Query Object Notation (SQON) - C API
 *  Copyright (C) 2015 Delwink, LLC
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published by
 *  the Free Software Foundation.
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
 * @date 01/03/2014
 * @author David McMackins II
 * @brief C implementation for Delwink's SQON
 */

#ifndef DELWINK_SQON_H
#define DELWINK_SQON_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief libsqon software version
 */
#define SQON_VERSION "0.0.0"

/**
 * @brief Information about the libsqon copyright holders and license.
 */
#define SQON_COPYRIGHT \
"libsqon - C API for Delwink's Structured Query Object Notation\n"\
"Copyright (C) 2015 Delwink, LLC\n\n"\
"This program is free software: you can redistribute it and/or modify\n"\
"it under the terms of the GNU Affero General Public License as published by\n"\
"the Free Software Foundation.\n\n"\
"This program is distributed in the hope that it will be useful,\n"\
"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"\
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"\
"GNU Affero General Public License for more details.\n\n"\
"You should have received a copy of the GNU Affero General Public License\n"\
"along with this program.  If not, see <http://www.gnu.org/licenses/>."

#define SQON_LOADERROR   -10

#define SQON_TYPEERROR   -11

#define SQON_MEMORYERROR -12

#define SQON_OVERFLOW    -13

#define SQON_UNSUPPORTED -14

#define SQON_INCOMPLETE  -15

#define SQON_CONNECTERR  -20

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Initializes SQON and supporting libraries.
 * @param qlen Maximum length for queries to the database.
 */
void sqon_init(size_t qlen);

/**
 * @brief The universal database connection auxiliary structure for libsqon.
 */
typedef struct dbconn {
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
sqon_dbsrv sqon_new_connection(uint8_t type, const char *host, const char *user,
        const char *passwd, const char *database);

/**
 * @brief Connects to the database server.
 * @param con Initialized database connection object.
 * @return Negative if input error; positive if connection error.
 */
int sqon_connect(sqon_dbsrv *srv);

/**
 * @brief Closes connection to the database server.
 * @param con Connected database connection object.
 */
void sqon_close(sqon_dbsrv *srv);

/**
 * @brief Query the database (SQL).
 * @param con Initialized database connection object.
 * @param query UTF-8 encoded SQL statement.
 * @param out Buffer into which to store the output (if any) as JSON.
 * @param n Buffer length of out.
 * @return Negative if input or IO error; positive if error from server.
 */
int sqon_query_sql(sqon_dbsrv *srv, const char *query, char *out, size_t n);

/**
 * @brief Query the database (SQON).
 * @param con Initialized database connection object.
 * @param query UTF-8 encoded SQON object with query instructions.
 * @param out Buffer into which to store the output (if any) as JSON.
 * @param n Bufer length of out.
 * @return Negative if input or IO error; positive if error from server.
 */
int sqon_query(sqon_dbsrv *srv, const char *query, char *out, size_t n);

#ifdef __cplusplus
}
#endif

#endif
