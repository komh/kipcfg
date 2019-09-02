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

class DHCPOptionParser
{
public :
    DHCPOptionParser() : mRouterList( NULL ), mDNSList( NULL ),
                         mDomainName( NULL ) {};
    ~DHCPOptionParser() { Free(); };

    bool Parse( struct dhcp_packet* dp );

    u_int8_t              GetMsgType() const { return mMsgType; };
    const struct in_addr& GetSID() const { return mSID; };
    u_int32_t             GetLeaseTime() const { return mLeaseTime; };
    const struct in_addr& GetSubnetMask() const { return mSubnetMask; };
    u_int8_t              GetRouterCount() const { return mRouterCount; };
    const struct in_addr* GetRouterList() const { return mRouterList; };
    u_int8_t              GetDNSCount() const { return mDNSCount; };
    const struct in_addr* GetDNSList() const { return mDNSList; };
    const char*           GetDomainName() const { return mDomainName; };

private :
    // a part of decoded options
    u_int8_t        mMsgType;
    struct in_addr  mSID;
    u_int32_t       mLeaseTime;
    struct in_addr  mSubnetMask;
    u_int8_t        mRouterCount;
    struct in_addr* mRouterList;
    u_int8_t        mDNSCount;
    struct in_addr* mDNSList;
    char          * mDomainName;

    void Free();
};
#endif

