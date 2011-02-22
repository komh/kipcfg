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

#include <stdlib.h>
#include <string.h>

#include <types.h>
#include <sys/socket.h> // struct sockaddr
#include <netinet/in.h> // struct sockaddr_in

#include "dhcp.h"
#include "log.h"

#include "dhcp_options.h"

struct dhcp_options *dhcp_options_parse( struct dhcp_packet *dp )
{
    struct dhcp_options *dopts;

    int code;
    int len;
    unsigned char *data;
    int i, j;

    if( memcmp( dp->options, DHCP_OPTIONS_COOKIE, 4 ))
        return NULL;

    dopts = calloc( 1, sizeof( *dopts ));

    for( i = 4; ( code = dp->options[ i++ ]) != DHO_END; )
    {
        if( code == DHO_PAD )
            continue;

        len  = dp->options[ i++ ];
        data = &dp->options[ i ];

        switch( code )
        {
            case DHO_DHCP_MESSAGE_TYPE :
                dopts->msg_type = *data;
                break;

            case DHO_DHCP_SERVER_IDENTIFIER :
                dopts->sid = *( struct in_addr * )data;
                break;

            case DHO_DHCP_LEASE_TIME :
                dopts->lease_time = *( u_int32_t * )data;
                break;

            case DHO_SUBNET_MASK :
                dopts->subnet_mask = *( struct in_addr * )data;
                break;

            case DHO_ROUTERS :
            {
                dopts->router_count = len / 4;
                dopts->router_list = malloc( sizeof( struct in_addr ) *
                                             dopts->router_count );
                for( j = 0; j < len; j += 4 )
                    dopts->router_list[ j / 4 ] = *( struct in_addr *)&data[ j ];
                break;
            }

            case DHO_DOMAIN_NAME_SERVERS :
            {
                dopts->dns_count = len / 4;
                dopts->dns_list = malloc( sizeof( struct in_addr ) *
                                          dopts->dns_count );
                for( j = 0; j < len; j += 4 )
                    dopts->dns_list[ j / 4 ] = *( struct in_addr *)&data[ j ];
                break;
            }

            case DHO_DOMAIN_NAME :
                dopts->domain_name = calloc( 1, len + 1 );
                memcpy( dopts->domain_name, data, len );
                break;
        }

        i += len;
    }

    return dopts;
}

void dhcp_options_free( struct dhcp_options *dopts )
{
    if( dopts )
    {
        free( dopts->router_list );
        free( dopts->dns_list );
        free( dopts->domain_name );
    }

    free( dopts );
}

