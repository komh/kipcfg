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

#ifndef KIPCFG_DAEMON_H
#define KIPCFG_DAEMON_H

#include <os2.h>

#define DHCPC_DAEMON_MAGIC  "I'm_daemon"

#define DCDM_CHECK_ALIVE 1
#define DCDM_REQUEST     2
#define DCDM_RELEASE     3
#define DCDM_QUIT        999

#define DCDE_NO_ERROR               0
#define DCDE_INVALID_MESSAGE        1
#define DCDE_PIPE_ERROR             2
#define DCDE_INVALID_INTERFACE      3
#define DCDE_IP_ALREADY_ASSIGNED    4
#define DCDE_IP_NOT_ASSIGNED        5
#define DCDE_IP_ASSIGNING           6
#define DCDE_IP_RELEASING           7
#define DCDE_SOCKET_ERROR           8
#define DCDE_REPLACE_RESOLV2_FAILED 9
#define DCDE_WAIT_TIME_OUT          10

#define DHCPC_STATE_INIT        1
#define DHCPC_STATE_INIT_REBOOT 2
#define DHCPC_STATE_SELECTING   3
#define DHCPC_STATE_RENEWING    4
#define DHCPC_STATE_REBINDING   5
#define DHCPC_STATE_BOUND       6

#pragma pack( 1 )
struct daemon_msg
{
    int msg;
    int arg;
    int iponly;
    int wait;
};
#pragma pack()

class Daemon
{
public :
    int  Main();
    void Start( const char *kipcfg_exe );
    int  Call( struct daemon_msg *dm );
    bool Alive();
};

class DaemonIFSem
{
public :
    bool mInitSuccess;

    DaemonIFSem( int ifnum, bool create = true );
    ~DaemonIFSem();

    int  Wait( int secs );
    void Post();

private :
    int mIFNum;
    HEV mHEV;
};
#endif

