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

#ifndef DELWINK_SQON_SQLQUERY_H
#define DELWINK_SQON_SQLQUERY_H

#include <mysql/mysql.h>
#include <postgresql/libpq-fe.h>
#include <stdint.h>

union res
{
  MYSQL_RES *mysql;
  PGresult *postgres;
};

union fields
{
  MYSQL_FIELD *mysql;
  PGresult *postgres;
};

union row
{
  MYSQL_ROW mysql;
  int postgres;
};

int
res_to_json (uint8_t type, void *res, char **out, const char *pk);

#endif
