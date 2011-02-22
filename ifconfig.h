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

#ifndef KIPCFG_IFCONFIG_H
#define KIPCFG_IFCONFIG_H

#include <types.h>

#define IFNUM_LOOPBACK 9

#ifndef INADDR_LOOPBACK
#define INADDR_LOOPBACK 0x7F000001
#endif

#define NETMASK_HOST 0xFFFFFFFF

struct ifconfig;

struct ifconfig *ifconfig_init( int ifnum, int broadcast );
void   ifconfig_done( struct ifconfig *ifc );
int    ifconfig_get( struct ifconfig *ifc, u_long *addr, u_long *netmask );
int    ifconfig_set( struct ifconfig *ifc, u_long addr, u_long netmask );

#endif
