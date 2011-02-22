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

#include <string.h>

#include <types.h>
#include <sys/socket.h> // socket()
#include <unistd.h>     // soclose()
#include <netinet/in.h> // IPPROTO_UDP

#include "ifconfig.h"
#include "log.h"

#include "dhcp_socks.h"

int dhcp_socks_init( struct dhcp_socks *ds )
{
    int    flag;
    struct sockaddr_in client;
    struct ifconfig   *ifc;

    ds->server = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP );
    if( ds->server < 0 )
    {
        log_msg("socket() for server failed : %s\n",
                sock_strerror( sock_errno()));

        return -1;
    }

    flag = 1;
    if( setsockopt( ds->server, SOL_SOCKET, SO_BROADCAST,
                    &flag, sizeof( flag )) < 0 )
    {
        log_msg("setsockopt(SO_BROADCAST) failed : %s\n",
                sock_strerror( sock_errno()));

        return -1;
    }

    ds->client = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP );
    if( ds->client < 0 )
    {
        log_msg("socket() for client failed : %s\n",
                sock_strerror( sock_errno()));

        return -1;
    }

    // without lo interface(LOOPBACK), IP broadcast does not work.
    // I don't know why
    ifc = ifconfig_init( IFNUM_LOOPBACK, 0 );
    ifconfig_set( ifc, htonl( INADDR_LOOPBACK ), htonl( NETMASK_HOST ));
    ifconfig_done( ifc );

    memset( &client, 0, sizeof( client ));
    client.sin_len         = sizeof( client );
    client.sin_family      = PF_INET;
    client.sin_addr.s_addr = htonl( INADDR_ANY );
    client.sin_port        = htons( CLIENT_PORT );

    if( bind( ds->client, ( struct sockaddr * )&client, sizeof( client )) < 0 )
    {
        log_msg("bind() failed : %s\n", sock_strerror( sock_errno()));

        return -1;
    }

    return 0;
}

void dhcp_socks_done( struct dhcp_socks *ds )
{
    soclose( ds->server );
    soclose( ds->client );
}

