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

#ifndef KIPCFG_DHCPC_H
#define KIPCFG_DHCPC_H

#include "dhcp.h"
#include "dhcp_socks.h"


int dhcp_discover( int ifnum, struct dhcp_socks *ds, struct dhcp_packet *dp );
int dhcp_request( int ifnum, struct dhcp_socks *ds, struct dhcp_packet *dp,
                  int state );
int dhcp_release( int ifnum, struct dhcp_socks *ds, struct dhcp_packet *dp );
int dhcp_decline( int ifnum, struct dhcp_socks *ds, struct dhcp_packet *dp );

#endif
