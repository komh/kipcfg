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

bool DHCPOptionParser::Parse( struct dhcp_packet *dp )
{
    int code;
    int len;
    unsigned char *data;
    int i, j;

    if( memcmp( dp->options, DHCP_OPTIONS_COOKIE, 4 ))
        return false;

    Free();

    for( i = 4; ( code = dp->options[ i++ ]) != DHO_END; )
    {
        if( code == DHO_PAD )
            continue;

        len  = dp->options[ i++ ];
        data = &dp->options[ i ];

        switch( code )
        {
            case DHO_DHCP_MESSAGE_TYPE :
                mMsgType = *data;
                break;

            case DHO_DHCP_SERVER_IDENTIFIER :
                mSID = *reinterpret_cast< struct in_addr * >( data );
                break;

            case DHO_DHCP_LEASE_TIME :
                mLeaseTime = *reinterpret_cast< u_int32_t * >( data );
                break;

            case DHO_SUBNET_MASK :
                mSubnetMask = *reinterpret_cast< struct in_addr * >( data );
                break;

            case DHO_ROUTERS :
            {
                mRouterCount = static_cast< u_int8_t >( len / 4 );
                mRouterList  = new struct in_addr[ mRouterCount ];
                for( j = 0; j < len; j += 4 )
                    mRouterList[ j / 4 ] = *reinterpret_cast
                                                < struct in_addr *>
                                                    ( &data[ j ]);
                break;
            }

            case DHO_DOMAIN_NAME_SERVERS :
            {
                mDNSCount = static_cast< u_int8_t >( len / 4 );
                mDNSList  = new struct in_addr[ mDNSCount ];
                for( j = 0; j < len; j += 4 )
                    mDNSList[ j / 4 ] = *reinterpret_cast
                                            < struct in_addr *>( &data[ j ]);
                break;
            }

            case DHO_DOMAIN_NAME :
                mDomainName = new char[ len + 1 ]();
                memcpy( mDomainName, data, len );
                break;
        }

        i += len;
    }

    return true;
}


void DHCPOptionParser::Free()
{
    delete[] mRouterList;
    delete[] mDNSList;
    delete[] mDomainName;

    mRouterList = NULL;
    mDNSList    = NULL;
    mDomainName = NULL;
}

