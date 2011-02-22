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

#ifndef KIPCFG_DHCP_OPTIONS_H
#define KIPCFG_DHCP_OPTIONS_H

#include <types.h>

#pragma pack( 1 )
struct dhcp_options
{
    // a part of decoded options
    u_int8_t        msg_type;
    struct in_addr  sid;
    u_int32_t       lease_time;
    struct in_addr  subnet_mask;
    u_int8_t        router_count;
    struct in_addr *router_list;
    u_int8_t        dns_count;
    struct in_addr *dns_list;
    char           *domain_name;
};
#pragma pack()

struct dhcp_options *dhcp_options_parse( struct dhcp_packet *dp );
void   dhcp_options_free( struct dhcp_options *dopts );

#endif

