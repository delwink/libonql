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

#ifndef DELWINK_SQON_H
#define DELWINK_SQON_H

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Initializes SQON and supporting libraries.
 * @param qlen Maximum length for queries to the database.
 */
void sqon_init(size_t qlen);

#ifdef __cplusplus
}
#endif

#endif
