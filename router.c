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
#include <process.h>

#include <types.h>
#include <sys/socket.h> // struct sockaddr
#include <unistd.h>     // soclose()
#include <netinet/in.h> // struct in_addr
#include <net/if.h>     // IFNAMSIZ
#include <arpa/inet.h>  // inet_ntoa()
#include <net/route.h>  // struct ortentry
#include <sys/ioctl.h>  // ioctl()

#include "log.h"

#include "router.h"

#pragma pack( 1 )
struct router
{
    int s;
};

struct rtentry
{
    struct ortentry ortentry;
    char   if_name[ IFNAMSIZ ];
};
#pragma pack()

struct router *router_init( int ifnum )
{
    struct router *r;

    r = ( struct router * )calloc( 1, sizeof( *r ));

    r->s = socket( PF_INET, SOCK_RAW, 0 );

    return r;
}

void router_done( struct router *r )
{
    soclose( r->s );

    free( r );
}

int router_add( struct router *r, u_long d, u_long g )
{
    struct rtentry      rte;
    struct sockaddr_in *sin;

    memset( &rte, 0, sizeof( rte ));

    sin = ( struct sockaddr_in * )&rte.ortentry.rt_dst;
    sin->sin_family = AF_INET;
    sin->sin_addr.s_addr = d;

    sin = ( struct sockaddr_in * )&rte.ortentry.rt_gateway;
    sin->sin_family = AF_INET;
    sin->sin_addr.s_addr = g;

    rte.ortentry.rt_flags = RTF_UP | RTF_STATIC;

    if( !d )  // default gateway ?
        rte.ortentry.rt_flags |= RTF_GATEWAY;

    return ioctl( r->s, SIOCADDRT, &rte );
}

int router_delete_ip( struct router *r, u_long d, u_long g, int flag )
{
    struct rtentries rtentries;
    struct rtentry  *rtentry;
    struct rtentry   rte;
    struct sockaddr_in *dst;
    struct sockaddr_in *gateway;
    struct sockaddr_in *sin;
    int    i;

    memset( &rtentries, 0, sizeof( rtentries ));

    if( os2_ioctl( r->s, SIOSTATRT, ( caddr_t )&rtentries,
                   sizeof( rtentries )) < 0 )
        return -1;

    rtentry = ( struct rtentry * )rtentries.rttable;
    for( i = 0; i < rtentries.hostcount + rtentries.netcount; i++ )
    {
        dst     = ( struct sockaddr_in * )&rtentry->ortentry.rt_dst;
        gateway = ( struct sockaddr_in * )&rtentry->ortentry.rt_gateway;

        if( d == dst->sin_addr.s_addr &&
            ( !g || g == gateway->sin_addr.s_addr ))
        {
            memset( &rte, 0, sizeof( rte ));

            sin = ( struct sockaddr_in * )&rte.ortentry.rt_dst;
            sin->sin_family = AF_INET;
            sin->sin_addr   = dst->sin_addr;

            sin = ( struct sockaddr_in * )&rte.ortentry.rt_gateway;
            sin->sin_family = AF_INET;
            sin->sin_addr   = gateway->sin_addr;

            if( ioctl( r->s, SIOCDELRT, &rte ) < 0 )
                return -1;

            if( flag != ROUTER_DELETE_ALL )
                break;
        }

        rtentry = ( struct rtentry * )(( char * )rtentry +
                                       sizeof( rtentry->ortentry ) +
                                       strlen( rtentry->if_name ) + 1 );
    }

    return 0;
}

int router_delete_if( struct router *r, int ifnum )
{
    return 0;
}


