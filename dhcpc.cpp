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
#include <netinet/in.h> // struct in_addr
#include <sys/ioctl.h>  // os2_ioctl()
#include <sys/socket.h> // struct sockaddr
#include <net/if.h>     // struct ifmib
#include <arpa/inet.h>  // inet_aton()
#include <sys/select.h>
#include <unistd.h>     // select()

#include "daemon.h"
#include "dhcp.h"
#include "dhcp_options.h"
#include "dhcp_socks.h"
#include "log.h"

#include "dhcpc.h"

// in seconds
#define DHCP_REPLY_WAIT_TIME    5

static const char *dhcp_msg_type_str[] = {
    "DHCP message type string",
    "DHCPDISCOVER",
    "DHCPOFFER",
    "DHCPREQUEST",
    "DHCPDECLINE",
    "DHCPACK",
    "DHCPNAK",
    "DHCPRELEASE",
    "DHCPINFORM",
    "DHCPLEASEQUERY",
    "DHCPLEASEUNASSIGNED",
    "DHCPLEASEUNKNOWN",
    "DHCPLEASEACTIVE",
};

static void get_hw_info( int ifnum, const DHCPSocks& ds, int *type, char *mac )
{
    struct ifmib ifmib;

    memset( &ifmib, 0, sizeof( ifmib ));
    os2_ioctl( ds.GetClient(), SIOSTATIF, ( caddr_t )&ifmib,
               sizeof( ifmib ));

    if( type )
        *type = ifmib.iftable[ ifnum ].iftType;

    if( mac )
        memcpy( mac, ifmib.iftable[ ifnum ].iftPhysAddr, 6 );
}

static int send_message( int ifnum, const DHCPSocks& ds, u_int8_t msg_type,
                         struct dhcp_packet *packet, int state )
{
    struct sockaddr_in server;
    struct dhcp_packet dp;
    DHCPOptionParser   dopts;
    int i;

    server.sin_len         = sizeof( server );
    server.sin_family      = AF_INET;
    server.sin_addr.s_addr = htonl( INADDR_BROADCAST );
    server.sin_port        = htons( SERVER_PORT );

    dopts.Parse( packet );

    memset( &dp, 0, sizeof( dp ));

    dp.op    = BOOTREQUEST;
    dp.htype = HTYPE_ETHER;
    dp.hlen  = 6;                   // ethernet mac addr length
    dp.xid   = packet->xid;
    dp.flags = packet->flags;

    get_hw_info( ifnum, ds, NULL, reinterpret_cast< char * >( dp.chaddr ));

    memcpy( dp.options, DHCP_OPTIONS_COOKIE, 4 );
    i = 4;
    dp.options[ i++ ] = DHO_DHCP_MESSAGE_TYPE;
    dp.options[ i++ ] = 1;          // length
    dp.options[ i++ ] = msg_type;

    switch( msg_type )
    {
        case DHCPREQUEST :
            switch( state )
            {
                case DHCPC_STATE_INIT :
                    dp.options[ i++ ] = DHO_DHCP_REQUESTED_ADDRESS;
                    dp.options[ i++ ] = 4;          // length
                    *reinterpret_cast< struct in_addr * >
                        ( &dp.options[ i ]) = packet->yiaddr;
                    i += 4;

                    dp.options[ i++ ] = DHO_DHCP_SERVER_IDENTIFIER;
                    dp.options[ i++ ] = 4;          // length
                    *reinterpret_cast< struct in_addr * >
                        ( &dp.options[ i ]) = dopts.GetSID();
                    i += 4;
                    break;

                case DHCPC_STATE_RENEWING :
                    server.sin_addr = dopts.GetSID();
                    // fall through

                case DHCPC_STATE_REBINDING :
                    dp.ciaddr = packet->yiaddr;
                    break;
            }
            break;

        case DHCPRELEASE :
            server.sin_addr = dopts.GetSID();

            dp.ciaddr = packet->yiaddr;

            dp.options[ i++ ] = DHO_DHCP_SERVER_IDENTIFIER;
            dp.options[ i++ ] = 4;          // length
            *reinterpret_cast< struct in_addr * >( &dp.options[ i ]) =
                dopts.GetSID();
            i += 4;
            break;

        case DHCPDECLINE :
        {
            dp.options[ i++ ] = DHO_DHCP_REQUESTED_ADDRESS;
            dp.options[ i++ ] = 4;          // length
            *reinterpret_cast< struct in_addr * >( &dp.options[ i ]) =
                packet->yiaddr.s_addr ? packet->yiaddr : packet->ciaddr;
            i += 4;

            dp.options[ i++ ] = DHO_DHCP_SERVER_IDENTIFIER;
            dp.options[ i++ ] = 4;          // length
            *reinterpret_cast< struct in_addr * >( &dp.options[ i ]) =
                dopts.GetSID();
            i += 4;
            break;
        }
    }

    dp.options[ i++ ] = DHO_DHCP_PARAMETER_REQUEST_LIST;
    dp.options[ i++ ] = 5;          // length
    dp.options[ i++ ] = DHO_DHCP_LEASE_TIME;
    dp.options[ i++ ] = DHO_SUBNET_MASK;
    dp.options[ i++ ] = DHO_ROUTERS;
    dp.options[ i++ ] = DHO_DOMAIN_NAME_SERVERS;
    dp.options[ i++ ] = DHO_DOMAIN_NAME;

    dp.options[ i++ ] = DHO_END;

    log_msg("lan%d : Sending %s(xid %x) message to %s...\n",
            ifnum,
            dhcp_msg_type_str[ msg_type ], packet->xid,
            inet_ntoa( server.sin_addr ));

    if( sendto( ds.GetServer(), &dp, sizeof( dp ), 0,
                reinterpret_cast< struct sockaddr * >( &server ),
                sizeof( server )) < 0 )
    {
        log_msg("sendto() failed : %s\n", sock_strerror( sock_errno()));

        return -1;
    }

    return 0;
}

static int wait_reply( const DHCPSocks& ds, struct dhcp_packet *dp, int sec )
{
    struct sockaddr_in client;
    int name_len;
    fd_set readfds;
    struct timeval tv;
    int rc;

    FD_ZERO( &readfds ); FD_SET( ds.GetClient(), &readfds );

    tv.tv_sec  = sec;
    tv.tv_usec = 0;

    rc = select( ds.GetClient() + 1, &readfds, NULL, NULL, &tv );

    if( rc < 0 )
    {
        log_msg("select() failed : %s\n", sock_strerror( sock_errno()));

        return -1;
    }

    if( rc == 0 )
    {
        log_msg("select() %d seconds timed out\n", sec );

        return 0;
    }

    name_len = sizeof( client );
    if( recvfrom( ds.GetClient(), dp, sizeof( *dp ), 0,
                  reinterpret_cast< struct sockaddr * >( &client ),
                  &name_len ) < 0 )
    {
        log_msg("recvfrom() failed : %s\n", sock_strerror( sock_errno()));

        return -1;
    }

    return 1;
}

static int check_reply( struct dhcp_packet *dp,
                        u_int8_t want_msg_type, u_int32_t want_xid )
{
    DHCPOptionParser dopts;
    u_int8_t         msg_type;
    struct           in_addr sid;

    dopts.Parse( dp );

    msg_type = dopts.GetMsgType();
    sid      = dopts.GetSID();

    if( msg_type == want_msg_type && dp->xid == want_xid )
        return 0;

    log_msg("Ooops, received %s(xid %x) not %s(xid %x) from %s. Ignore.\n",
            dhcp_msg_type_str[ msg_type ], dp->xid,
            dhcp_msg_type_str[ want_msg_type ], want_xid,
            inet_ntoa( sid ));

    return -1;
}

static u_int32_t make_xid( void )
{
    struct timeval tv;

    gettimeofday( &tv, NULL );

    return tv.tv_sec + tv.tv_usec;
}

int DHCPClient::Discover( struct dhcp_packet *dp )
{
    u_int32_t xid;
    struct    dhcp_packet packet;

    // select xid and save it in order to idenfity it later
    dp->xid   = xid = make_xid();
    dp->flags = htons( BOOTP_BROADCAST );

    if( send_message( mIFNum, mDS, DHCPDISCOVER, dp, 0 ) < 0 )
        return -1;

    do
    {
        packet = *dp;

        log_msg("lan%d : Waiting DHCPOFFER(xid %x)...\n", mIFNum, xid );
        if( wait_reply( mDS, &packet, DHCP_REPLY_WAIT_TIME ) <= 0 )
            return -1;  // error or timeout
    } while( check_reply( &packet, DHCPOFFER, xid ));

    *dp = packet;

    return 0;
}

int DHCPClient::Request( struct dhcp_packet *dp, int state )
{
    u_int32_t xid;
    struct    dhcp_packet packet;

    // save xid from DHCPOFFER provided by server to identify it later
    // on INIT state, otherwise generate it, then pass it to server again
    if( state != DHCPC_STATE_INIT )
        dp->xid = make_xid();

    xid       = dp->xid;
    dp->flags = htons( BOOTP_BROADCAST );

    if( send_message( mIFNum, mDS, DHCPREQUEST, dp, state ) < 0 )
        return -1;

    do
    {
        packet = *dp;

        log_msg("lan%d : Waiting DHCPACK(xid %x)...\n", mIFNum, xid );
        if( wait_reply( mDS, &packet, DHCP_REPLY_WAIT_TIME ) <= 0 )
            return -1;  // error or time out
    } while( check_reply( &packet, DHCPACK, xid ));

    *dp = packet;

    return 0;
}

int DHCPClient::Release( struct dhcp_packet *dp )
{
    dp->xid   = make_xid();
    dp->flags = 0;
    if( send_message( mIFNum, mDS, DHCPRELEASE, dp, 0 ) < 0 )
        return -1;

    return 0;
}

int DHCPClient::Decline( struct dhcp_packet *dp )
{
    u_int32_t xid;

    // select xid and save it in order to idenfity it later
    dp->xid   = xid = make_xid();
    dp->flags = 0;
    if( send_message( mIFNum, mDS, DHCPDECLINE, dp, 0 ) < 0 )
        return -1;

    return 0;
}

