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

#ifndef SO_L_BROADCAST
#define SO_L_BROADCAST 0x0200   // limited broadcast sent on all IFs
#endif

// SO_L_BROADCAST does two things on the OS/2 TCP/IP stack(AFINET.SYS)
//   - send : sendto() 255.255.255.255 goes out of all the interfaces which
//            are up, without routing table and without rewriting the
//            destination to the broadcast address of the primary interface
//   - recv : recvfrom() stores the IPv4 address of the receiving interface
//            into the first 4 bytes of sin_zero of the source address
static int set_l_broadcast( int s, const char *name )
{
    int flag = 1;

    if( setsockopt( s, SOL_SOCKET, SO_L_BROADCAST,
                    &flag, sizeof( flag )) < 0 )
    {
        log_msg("setsockopt(SO_L_BROADCAST) for %s failed : %s\n",
                name, sock_strerror( sock_errno()));

        return -1;
    }

    return 0;
}

DHCPSocks::DHCPSocks()
{
    int    flag;
    struct sockaddr_in client;

    mInitSuccess = false;

    mServer = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP );
    if( mServer < 0 )
    {
        log_msg("socket() for server failed : %s\n",
                sock_strerror( sock_errno()));

        return;
    }

    flag = 1;
    if( setsockopt( mServer, SOL_SOCKET, SO_BROADCAST,
                    &flag, sizeof( flag )) < 0 )
    {
        log_msg("setsockopt(SO_BROADCAST) failed : %s\n",
                sock_strerror( sock_errno()));

        return;
    }

    if( set_l_broadcast( mServer, "server") < 0 )
        return;

    mClient = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP );
    if( mClient < 0 )
    {
        log_msg("socket() for client failed : %s\n",
                sock_strerror( sock_errno()));

        return;
    }

    // to know which interface received a reply
    if( set_l_broadcast( mClient, "client") < 0 )
        return;

    // without lo interface(LOOPBACK), IP broadcast does not work.
    // I don't know why
    IFConfig ifc( IFNUM_LOOPBACK, 0 );
    ifc.Set( htonl( INADDR_LOOPBACK ), htonl( NETMASK_HOST ));

    memset( &client, 0, sizeof( client ));
    client.sin_len         = sizeof( client );
    client.sin_family      = PF_INET;
    client.sin_addr.s_addr = htonl( INADDR_ANY );
    client.sin_port        = htons( CLIENT_PORT );

    if( bind( mClient, reinterpret_cast< struct sockaddr * >( &client ),
              sizeof( client )) < 0 )
    {
        log_msg("bind() failed : %s\n", sock_strerror( sock_errno()));

        return;
    }

    mInitSuccess = true;
}

DHCPSocks::~DHCPSocks()
{
    soclose( mServer );
    soclose( mClient );
}

