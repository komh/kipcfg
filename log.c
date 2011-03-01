/*
 * This file is part of kipcfg.
 *
 * kipcfg is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * kipcfg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with kipcfg; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#include "log.h"

void log_msg( const char *format, ... )
{
    FILE   *fp;
    time_t  t;
    struct  tm tm;
    va_list arg;

    fp = fopen("kipcfg.log", "at");

    time( &t );

    tm = *localtime( &t );

    fprintf( fp, "%04d-%02d-%02d %02d:%02d:%02d ",
             tm.tm_year + 1900, tm.tm_mon, tm.tm_mday,
             tm.tm_hour, tm.tm_min, tm.tm_sec );

    va_start( arg, format );

    vfprintf( fp, format, arg );

    va_end( arg );

    fclose( fp );
}

