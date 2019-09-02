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

#ifndef KIPCFG_DHCP_SOCKS_H
#define KIPCFG_DHCP_SOCKS_H

#define SERVER_PORT 67
#define CLIENT_PORT 68

class DHCPSocks
{
public :
    bool mInitSuccess;

    DHCPSocks();
    ~DHCPSocks();

    int GetServer() const { return mServer; };
    int GetClient() const { return mClient; };

private :
    int mServer;
    int mClient;
};
#endif

