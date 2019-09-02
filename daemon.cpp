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

#define INCL_DOS
#define INCL_DOSERRORS
#include <os2.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <process.h>

#include <types.h>
#include <sys/socket.h> // struct sockaddr
#include <netinet/in.h> // struct in_addr
#include <arpa/inet.h>  // inet_ntoa
#include <sys/ioctl.h>  // ioctl()
#include <net/if.h>     // struct ifreq

#include "dhcp.h"
#include "dhcpc.h"
#include "dhcp_options.h"
#include "ifconfig.h"
#include "router.h"
#include "log.h"

#include "daemon.h"

#define KIPCFG_PIPE_NAME "\\PIPE\\KIPCFG"
#define KIPCFG_SEM_NAME  "\\SEM32\\KIPCFG"

#define IP_STATUS_NOT_ASSIGNED  0
#define IP_STATUS_ASSIGNING     1
#define IP_STATUS_ASSIGNED      2
#define IP_STATUS_RELEASING     3

#define IF_INFO_ENTRIES 9

// in milli-seconds
#define DHCPC_DAEMON_WAIT_TIME  ( 10 * 1000 )

#define STATE_LIMIT_TIME 60

#pragma pack( 1 )
struct if_info
{
    HMTX   hmtx;
    DHCPClient *dhcpc;
    int    ifnum;
    TID    tid;
    int    iponly;
    int    wait;
    int    ip_status;
    struct dhcp_packet dp;
    time_t start_time;
    time_t renewing_time;
    time_t rebinding_time;
    time_t expiration_time;
    int    state;
    int    quit;
};
#pragma pack()

static const char *dhcpc_state_str[] = {
    "DHCP Client state string",
    "INIT",
    "INIT-REBOOT",
    "SELECTING",
    "RENEWING",
    "REBINDING",
    "BOUND",
};

static int is_valid_interface( int ifnum, const DHCPSocks& ds )
{
    struct ifreq ifr;

    strcpy( ifr.ifr_name, "lan0");
    ifr.ifr_name[ 3 ] += static_cast< char >( ifnum );

    return ioctl( ds.GetClient(), SIOCGIFVALID, &ifr ) == 0;
}

static int replace_resolv2( const char *domain, int count,
                            const struct in_addr *list )
{
    const char *env_etc = getenv("ETC");
    char resolv2_path[ CCHMAXPATH ];
    FILE *resolv2;
    int i;

    if( count == 0 )
        return 0;

    resolv2_path[ 0 ] = '\0';
    if( env_etc )
    {
        strcpy( resolv2_path, env_etc );
        strcat( resolv2_path, "\\");
    }
    strcat( resolv2_path, "RESOLV2");

    resolv2 = fopen( resolv2_path, "wt");
    if( !resolv2 )
    {
        log_msg("RESOLV2(%s) open failed!!!\n", resolv2_path );

        return 1;
    }

    if( domain )
        fprintf( resolv2, "domain %s\n", domain );

    for( i = 0; i < count; i++ )
        fprintf( resolv2, "nameserver %s\n", inet_ntoa( list[ i ]));

    fclose( resolv2 );

    return 0;
}

static void ip_assign( void *arg )
{
    struct if_info *info = reinterpret_cast< struct if_info * >( arg );
    DaemonIFSem    *sem;
    struct in_addr in_addr;
    int    init_state;
    int    use_broadcast;
    time_t next_time;

    info->state = DHCPC_STATE_INIT;
    log_msg("lan%d : %s state\n", info->ifnum, dhcpc_state_str[ info->state ]);

    if( info->wait )
    {
        sem = new DaemonIFSem( info->ifnum, false );
        if( !sem->mInitSuccess )
        {
            delete sem;
            sem = NULL;
        }
    }

    Router r( info->ifnum );

    do
    {
        init_state = info->state == DHCPC_STATE_INIT;

        if( !init_state )
        {
            time_t current_time = time( NULL );

            if( current_time < next_time )
            {
                DosSleep( 1 );
                continue;
            }

            switch( info->state )
            {
                case DHCPC_STATE_BOUND :
                    info->state = DHCPC_STATE_RENEWING;
                    log_msg("lan%d : %s state\n",
                            info->ifnum, dhcpc_state_str[ info->state ]);

                    next_time = current_time +
                                ( info->rebinding_time - current_time ) / 2;
                    break;

                case DHCPC_STATE_RENEWING :
                    if( current_time < info->rebinding_time - STATE_LIMIT_TIME )
                    {
                        next_time = current_time +
                                    ( info->rebinding_time - current_time ) / 2;
                        break;
                    }

                    info->state = DHCPC_STATE_REBINDING;
                    log_msg("lan%d : %s state\n",
                            info->ifnum, dhcpc_state_str[ info->state ]);

                    next_time = current_time +
                                ( info->expiration_time - current_time ) / 2;
                    break;

                case DHCPC_STATE_REBINDING :
                    if( current_time < info->expiration_time - STATE_LIMIT_TIME )
                    {
                        next_time = current_time +
                                    ( info->expiration_time - current_time ) / 2;
                        break;
                    }

                    // Expired, restart INIT state
                    info->state = DHCPC_STATE_INIT;

                    log_msg("lan%d : Lease expired !!! Restart %s state\n",
                            info->ifnum, dhcpc_state_str[ info->state ]);
                    continue;
            }
        }

        // to prevent xid conflicts
        DosRequestMutexSem( info->hmtx, SEM_INDEFINITE_WAIT );

        // check if 'quit' was asked while waiting mutex
        if( info->quit )
        {
            DosReleaseMutexSem( info->hmtx );

            break;
        }

        IFConfig ifc( info->ifnum, init_state );

        use_broadcast = info->state == DHCPC_STATE_INIT ||
                        info->state == DHCPC_STATE_REBINDING;

        if( use_broadcast )
        {
            r.DeleteIP( htonl( ROUTER_BROADCAST ), 0, ROUTER_DELETE_ALL );

            ifc.Get( &in_addr.s_addr, NULL );
            r.Add( htonl( ROUTER_BROADCAST ), in_addr.s_addr );
        }

        if(( !init_state || info->dhcpc->Discover( &info->dp ) == 0 ) &&
           info->dhcpc->Request( &info->dp, info->state ) == 0 )
        {
            DHCPOptionParser dopts;
            int       i;

            dopts.Parse( &info->dp );

            log_msg("lan%d : IP configurations\n", info->ifnum );
            log_msg("\tIP address = %s\n", inet_ntoa( info->dp.yiaddr ));
            log_msg("\tSubnet mask = %s\n",
                    inet_ntoa( dopts.GetSubnetMask()));
            log_msg("\tLease times = %lu seconds\n", dopts.GetLeaseTime());
            if( !info->iponly )
            {
                log_msg("\tRouters\n");
                for( i = 0; i < dopts.GetRouterCount(); i++ )
                    log_msg("\t\tno. %d = %s\n", i + 1,
                            inet_ntoa( dopts.GetRouterList()[ i ]));
                log_msg("\tDNS servers\n");
                for( i = 0; i < dopts.GetDNSCount(); i++ )
                    log_msg("\t\tno. %d = %s\n", i + 1,
                            inet_ntoa( dopts.GetDNSList()[ i ]));
                log_msg("\tDomain name = %s\n", dopts.GetDomainName());
            }
            log_msg("\tDHCP server = %s\n", inet_ntoa( dopts.GetSID()));

            if( use_broadcast )
                r.DeleteIP( htonl( ROUTER_BROADCAST ), 0, ROUTER_DELETE_ALL );

            if( init_state )
            {
                ifc.Set( info->dp.yiaddr.s_addr,
                         dopts.GetSubnetMask().s_addr );

                if( !info->iponly )
                {
                    for( i = 0; i < dopts.GetRouterCount(); i++ )
                        r.Add( htonl( ROUTER_DEFAULT ),
                               dopts.GetRouterList()[ i ].s_addr );

                    replace_resolv2( dopts.GetDomainName(),
                                     dopts.GetDNSCount(),
                                     dopts.GetDNSList());
                }
            }

            info->start_time      =
            info->renewing_time   =
            info->rebinding_time  =
            info->expiration_time = time( NULL );

            info->renewing_time   += dopts.GetLeaseTime() / 2;
            info->rebinding_time  += dopts.GetLeaseTime() * 7 / 8;
            info->expiration_time += dopts.GetLeaseTime();

            next_time = info->renewing_time;

            info->ip_status = IP_STATUS_ASSIGNED;

            info->state = DHCPC_STATE_BOUND;
            log_msg("lan%d : %s state\n", info->ifnum,
                    dhcpc_state_str[ info->state ]);

            if( info->wait && sem )
            {
                sem->Post();

                delete sem;
                sem = NULL;
            }
        }

        DosReleaseMutexSem( info->hmtx );
    } while( !info->quit );
}

static void ip_release( struct if_info *info )
{
    info->ip_status = IP_STATUS_RELEASING;

    info->dhcpc->Release( &info->dp );
    delete info->dhcpc;

    info->quit = 1;
    DosWaitThread( &info->tid, DCWW_WAIT );

    IFConfig ifc( info->ifnum, 0 );
    ifc.Set( htonl( INADDR_ANY ), htonl( NETMASK_HOST ));

    // set all to 0, as a result, ip_status becomes IP_STATUS_NOT_ASSIGNED
    memset( info, 0, sizeof( *info ));
}

int Daemon::Main()
{
    struct if_info if_info_table[ IF_INFO_ENTRIES ];
    HMTX   hmtx;
    HPIPE  hpipe;
    struct daemon_msg dm;
    ULONG  cbActual;
    HEV    hev = NULLHANDLE;
    int    quit = 0;

    log_msg("----- kipcfg daemon started -----\n");

    DosCreateNPipe( KIPCFG_PIPE_NAME, &hpipe, NP_ACCESS_DUPLEX,
                    NP_TYPE_MESSAGE | NP_READMODE_MESSAGE | 1, 1024, 1024, 0 );

    DosCreateMutexSem( NULL, &hmtx, 0, FALSE );

    // send ack
    DosOpenEventSem( KIPCFG_SEM_NAME, &hev );
    DosPostEventSem( hev );
    DosCloseEventSem( hev );

    DHCPSocks ds;
    if( !ds.mInitSuccess )
    {
        log_msg("DHCPSocks() failed!!!\n");

        DosCloseMutexSem( hmtx );

        DosClose( hpipe );

        return 1;
    }

    memset( if_info_table, 0, sizeof( if_info_table ));

    do
    {
        DosConnectNPipe( hpipe );

        // receive message
        DosRead( hpipe, &dm, sizeof( dm ), &cbActual );

        switch( dm.msg )
        {
            case DCDM_CHECK_ALIVE :
                dm.msg = DCDE_NO_ERROR;
                break;

            case DCDM_REQUEST :
            {
                struct if_info *info = &if_info_table[ dm.arg ];

                if( !is_valid_interface( dm.arg, ds ))
                {
                    dm.msg = DCDE_INVALID_INTERFACE;
                    break;
                }

                switch( info->ip_status )
                {
                    case IP_STATUS_ASSIGNING :
                        dm.msg = DCDE_IP_ASSIGNING;
                        break;

                    case IP_STATUS_ASSIGNED :
                        dm.msg = DCDE_IP_ALREADY_ASSIGNED;

                        if( dm.wait )
                        {
                            DaemonIFSem sem( dm.arg, false );
                            sem.Post();
                        }
                        break;

                    case IP_STATUS_RELEASING :
                        dm.msg = DCDE_IP_RELEASING;
                        break;

                    case IP_STATUS_NOT_ASSIGNED :
                        info->ip_status = IP_STATUS_ASSIGNING;
                        info->hmtx      = hmtx;
                        info->ifnum     = dm.arg;
                        info->iponly    = dm.iponly;
                        info->wait      = dm.wait;
                        info->dhcpc     = new DHCPClient( info->ifnum, ds );
                        info->tid       = _beginthread( ip_assign, NULL,
                                                        1024 * 1024, info );

                        dm.msg = DCDE_NO_ERROR;
                        break;
                }
                break;
            }

            case DCDM_RELEASE :
            {
                if( !is_valid_interface( dm.arg, ds ))
                    dm.msg = DCDE_INVALID_INTERFACE;
                else
                {
                    struct if_info *info = &if_info_table[ dm.arg ];

                    switch( info->ip_status )
                    {
                        case IP_STATUS_NOT_ASSIGNED :
                            dm.msg = DCDE_IP_NOT_ASSIGNED;
                            break;

                        case IP_STATUS_ASSIGNING :
                            dm.msg = DCDE_IP_ASSIGNING;
                            break;

                        case IP_STATUS_RELEASING :
                            dm.msg = DCDE_IP_RELEASING;
                            break;

                        case IP_STATUS_ASSIGNED :
                            ip_release( info );

                            dm.msg = DCDE_NO_ERROR;
                            break;
                    }
                }
                break;
            }

            case DCDM_QUIT :
            {
                struct if_info *info;
                int i;

                for( i = 0; i < IF_INFO_ENTRIES; i++ )
                {
                    info = &if_info_table[ i ];

                    if( info->tid )
                    {
                        info->quit = 1;

                        DosWaitThread( &info->tid, DCWW_WAIT );
                    }
                }

                quit = 1;

                dm.msg = DCDE_NO_ERROR;
                break;
            }

            default :
                dm.msg = DCDE_INVALID_MESSAGE;
                break;
        }

        // send result
        DosWrite( hpipe,  &dm, sizeof( dm ), &cbActual );

        // wait ack
        DosRead( hpipe, &dm, sizeof( dm ), &cbActual );

        DosDisConnectNPipe( hpipe );
    } while( !quit );

    DosCloseMutexSem( hmtx );

    DosClose( hpipe );

    return 0;
}

void Daemon::Start( const char *kipcfg_exe )
{
    HEV         hev;
    CHAR        szFailureName[ CCHMAXPATH ];
    RESULTCODES rc;

    DosCreateEventSem( KIPCFG_SEM_NAME, &hev, DC_SEM_SHARED, FALSE );

    DosExecPgm( szFailureName, sizeof( szFailureName ),
                EXEC_BACKGROUND, "kipcfg\0 "DHCPC_DAEMON_MAGIC"\0", NULL, &rc,
                kipcfg_exe );

    // wait ack
    DosWaitEventSem( hev, SEM_INDEFINITE_WAIT );
    DosCloseEventSem( hev );
}

int Daemon::Call( struct daemon_msg *dm )
{
    HPIPE hpipe;
    ULONG ulAction;
    ULONG cbActual;
    ULONG rc;

    do
    {
        rc = DosOpen( KIPCFG_PIPE_NAME, &hpipe, &ulAction, 0, 0,
                      OPEN_ACTION_OPEN_IF_EXISTS,
                      OPEN_ACCESS_READWRITE | OPEN_SHARE_DENYREADWRITE |
                      OPEN_FLAGS_FAIL_ON_ERROR,
                      NULL );

        if( rc == ERROR_PIPE_BUSY )
            DosWaitNPipe( KIPCFG_PIPE_NAME, 0 );

    } while( rc && rc != ERROR_FILE_NOT_FOUND && rc != ERROR_PATH_NOT_FOUND );

    if( rc )
    {
        dm->msg = DCDE_PIPE_ERROR;
        dm->arg = rc;

        return -1;
    }

    // send message
    DosWrite( hpipe, dm, sizeof( *dm ), &cbActual );

    // receive result
    DosRead( hpipe, dm, sizeof( *dm ), &cbActual );

    // send ack
    DosWrite( hpipe, dm, sizeof( *dm ), &cbActual );

    DosClose( hpipe );

    return 0;
}

bool Daemon::Alive()
{
#if 0
    struct daemon_msg dm;

    dm.msg  = DCDM_CHECK_ALIVE;
    daemon_call( &dm );

    return dm.msg == DCDE_NO_ERROR;
#else
    ULONG rc;

    rc = DosWaitNPipe( KIPCFG_PIPE_NAME, 0 );

    return ( rc != ERROR_FILE_NOT_FOUND && rc != ERROR_PATH_NOT_FOUND );
#endif
}

DaemonIFSem::DaemonIFSem( int ifnum, bool create )
{
    char sem_name[ 128 ];
    ULONG rc;

    sprintf( sem_name, "%s%d", KIPCFG_SEM_NAME, ifnum );

    if( create )
      rc = DosCreateEventSem( sem_name, &mHEV, DC_SEM_SHARED, FALSE );
    else
      rc = DosOpenEventSem( sem_name, &mHEV );

    mInitSuccess = !rc;
}

DaemonIFSem::~DaemonIFSem()
{
    DosCloseEventSem( mHEV );
}

int DaemonIFSem::Wait( int secs )
{
    if( DosWaitEventSem( mHEV, secs * 1000 ))
        return -1;

    return 0;
}

void DaemonIFSem::Post()
{
    DosPostEventSem( mHEV );
}

