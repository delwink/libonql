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
#include "internal.h"

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

int sqon_connect(sqon_dbsrv *srv)
{
    switch (srv->type) {
    case SQON_DBCONN_MYSQL:
        srv->com = mysql_init(NULL);
        if (NULL == mysql_real_connect(srv->com, srv->host, srv->user,
                srv->passwd, srv->database, 0, NULL, CLIENT_MULTI_STATEMENTS))
            return (int) mysql_errno(srv->com);
        break;

    default:
        return -1;
    }

    srv->isopen = true;
    return 0;
}

void sqon_close(sqon_dbsrv *srv)
{
    switch (srv->type) {
    case SQON_DBCONN_MYSQL:
        mysql_close(srv->com);
        srv->isopen = false;
        break;
    }
}

int sqon_query_sql(sqon_dbsrv *srv, const char *query, char *out, size_t n)
{
    int rc = 0;
    bool connected = srv->isopen;
    union {
        MYSQL_RES *mysql;
    } res;

    if (!connected) {
        rc = sqon_connect(srv);
        if (rc)
            return rc;
    }

    switch (srv->type) {
    case SQON_DBCONN_MYSQL:
        if (mysql_query(srv->com, query)) {
            rc = mysql_errno(srv->com);
            if (!connected)
                sqon_close(srv);
            return rc;
        }
        break;

    default:
        return -1;
    }

    if (NULL != out) {
        switch (srv->type) {
        case SQON_DBCONN_MYSQL:
            res.mysql = mysql_store_result(srv->com);
            if (!connected)
                sqon_close(srv);
            if (NULL == res.mysql)
                return -2;
            rc = res_to_json(SQON_DBCONN_MYSQL, res.mysql, out, n);
            mysql_free_result(res.mysql);
            break;

        default: /* this should be impossible */
            return -3;
        }
    } else if (!connected) {
        sqon_close(srv);
    }

    return rc;
}

int sqon_query(sqon_dbsrv *srv, const char *query, char *out, size_t n)
{
    char *sql = calloc(QLEN, sizeof(char));
    if (NULL == sql)
        return SQON_MEMORYERROR;

    int rc = sqon_to_sql(srv, query, sql, QLEN);
    if (!rc)
        rc = sqon_query_sql(srv, sql, out, n);
    free(sql);
    return rc;
}
