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
struct dbconn {
    void *con;
    bool isopen;
    uint8_t type;
    const char *host;
    const char *user;
    const char *passwd;
    const char *database;
};

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
struct dbconn sqon_new_connection(uint8_t type, const char *host,
        const char *user, const char *passwd, const char *database);

/**
 * @brief Connects to the database server.
 * @param con Initialized database connection object.
 * @return Negative if input error; positive if connection error.
 */
int sqon_connect(struct dbconn *con);

/**
 * @brief Closes connection to the database server.
 * @param con Connected database connection object.
 */
void sqon_close(struct dbconn *con);

#ifdef __cplusplus
}
#endif

#endif
