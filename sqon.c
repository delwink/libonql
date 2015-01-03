/*
 *  Structured Query Object Notation - C API
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

#include <jansson.h>
#include <mysql.h>

#include "sqon.h"
#include "safemem.h"

size_t QLEN = 0;

void sqon_init(size_t qlen)
{
    QLEN = qlen;
    json_set_alloc_funcs(safe_malloc, safe_free);
}

struct dbconn sqon_new_connection(uint8_t type, const char *host,
        const char *user, const char *passwd, const char *database)
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

int sqon_connect(struct dbconn *con)
{
    switch (con->type) {
        case SQON_DBCONN_MYSQL:
            con->con = mysql_init(NULL);
            if (NULL == mysql_real_connect(con->con, con->host, con->user,
                    con->passwd, con->database, 0, NULL, 0))
                return (int) mysql_errno(con->con);
            break;

        default:
            return -1;
    }

    con->isopen = true;
    return 0;
}

void sqon_close(struct dbconn *con)
{
    switch (con->type) {
        case SQON_DBCONN_MYSQL:
            mysql_close(con->con);
            con->isopen = false;
            break;
    }
}
