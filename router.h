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

#ifndef KIPCFG_ROUTER_H
#define KIPCFG_ROUTER_H

#define ROUTER_DEFAULT      0x00000000
#define ROUTER_BROADCAST    0xFFFFFFFF

#define ROUTER_DELETE_ONE   0
#define ROUTER_DELETE_ALL   1

struct router;

struct router *router_init( int ifnum );
void router_done( struct router *r );
int router_add( struct router *r, u_long d, u_long g );
int router_delete_ip( struct router *r, u_long d, u_long g, int flag );
int router_delete_if( struct router *r, int ifnum );

#endif

