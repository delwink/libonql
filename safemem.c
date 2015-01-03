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

#include "safemem.h"

static void *safe_memset(void *v, int c, size_t n)
{
    volatile char *p = v;
    while (n--)
        *p++ = c;

    return v;
}

void *safe_malloc(size_t n)
{
    /* Store the memory area size in the beginning of the block */
    void *v = malloc(n + sizeof(size_t));
    *((size_t *) v) = n;
    return v + sizeof(size_t);
}

void safe_free(void *v)
{
    size_t n;

    v -= sizeof(size_t);
    n = *((size_t *) v);

    safe_memset(v, 0, n + sizeof(size_t));
    free(v);
}
