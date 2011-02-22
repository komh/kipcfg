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
#include <string.h>
#include <stdlib.h>
#include <process.h>

#include <types.h>
#include <sys/socket.h> // socket()
#include <unistd.h>     // soclose()
#include <arpa/inet.h>  // inet_ntoa()
#include <net/if.h>     // IFNAMSIZ
#include <sys/ioctl.h>  // ioctl()

#include "router.h"
#include "log.h"

#include "ifconfig.h"

#pragma pack( 1 )
struct ifconfig
{
    int     ifnum;
    int     s;
    struct  router *r;
};
#pragma pack()

static const char *if_names[] = {"lan0", "lan1", "lan2", "lan3", "lan4",
                                 "lan5", "lan6", "lan7", "lan8", "lo", };

// TODO : replace with ioctl() and os2_ioctl()
//        But, 'route' command can cause a system trap when using them.
//        So use 'ifconfig.exe' until fixing them.
//
// What ifconfig.exe do
//   1. Remove the entries related to a interface of the routing table
//   2. Set IP address, netmask and broadcast address for a interface
//   3. Add a route table entry to a subnet using a interface
static int call_ifconfig( int ifnum, const char *addr, const char *netmask )
{
    const char *argv[ 6 ];
    int i;

    i = 0;
    argv[ i++ ] = "ifconfig.exe";
    argv[ i++ ] = if_names[ ifnum ];
    argv[ i++ ] = addr;
    if( netmask )
    {
        argv[ i++ ] = "netmask";
        argv[ i++ ] = netmask;
    }
    argv[ i ] = NULL;

    return spawnvp( P_WAIT, "ifconfig.exe", argv );
}

struct ifconfig *ifconfig_init( int ifnum, int assign_priv_ip )
{
    struct ifconfig *ifc;
    struct router   *r;
    int    s;
    char   if_ip[ IFNAMSIZ ];

    ifc = calloc( 1, sizeof( *ifc ));

    s = socket( PF_INET, SOCK_RAW, 0 );

    r = router_init( ifnum );

    if( assign_priv_ip )
    {
        struct ifreq ifreq;

        strcpy( ifreq.ifr_name, if_names[ ifnum ]);
        // IP address not assigned ?
        if( ioctl( s, SIOCGIFADDR, &ifreq ) < 0 ||
            !(( struct sockaddr_in * )&ifreq.ifr_addr )->sin_addr.s_addr )
        {
            log_msg("%s : Assign a private IP address\n", if_names[ ifnum ]);

            sprintf( if_ip, "172.%d.100.100", 16 + ifnum );
            call_ifconfig( ifnum, if_ip, "255.255.0.0");
        }
    }

    ifc->ifnum = ifnum;
    ifc->s     = s;
    ifc->r     = r;

    return ifc;
}

void ifconfig_done( struct ifconfig *ifc )
{
    router_done( ifc->r );

    soclose( ifc->s );

    free( ifc );
}

int ifconfig_get( struct ifconfig *ifc, u_long *addr, u_long *netmask )
{
    struct ifreq ifreq;

    if( addr )
    {
        strcpy( ifreq.ifr_name, if_names[ ifc->ifnum ]);
        if( ioctl( ifc->s, SIOCGIFADDR, &ifreq ) < 0 )
            return -1;

        *addr = (( struct sockaddr_in * )&ifreq.ifr_addr )->sin_addr.s_addr;
    }

    // TODO : get netmask

    return 0;
}

int ifconfig_set( struct ifconfig *ifc, u_long addr, u_long netmask )
{
    char addr_name[ IFNAMSIZ ];
    char mask_name[ IFNAMSIZ ];

    strcpy( addr_name, inet_ntoa( *( struct in_addr * )&addr ));
    strcpy( mask_name, inet_ntoa( *( struct in_addr * )&netmask ));

    return call_ifconfig( ifc->ifnum, addr_name, mask_name );
}


