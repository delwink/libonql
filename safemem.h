/*
 *  safemem - safer functions for managing sensitive data in memory
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

#ifndef DELWINK_SAFEMEM_H
#define DELWINK_SAFEMEM_H

#include <stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif

void *safe_malloc(size_t n);

void safe_free(void *v);

#ifdef __cplusplus
}
#endif

#endif
