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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "daemon.h"

#define KIPCFG_VER  "v0.1.0"

#define MODE_REQUEST    1
#define MODE_RELEASE    2

struct options
{
    int mode;
    int ifnum;
    int iponly;
    int wait;
    int quit;
};

static void show_usage( void )
{
    fprintf( stderr, "\n");
    fprintf( stderr, "Usage: kipcfg /req interface | /rel interface | /iponly | /wait secs | /q\n");
    fprintf( stderr, "\n");
    fprintf( stderr, "Options\n");
    fprintf( stderr, "\t/req    : request IP address for the specefied interface\n");
    fprintf( stderr, "\t/rel    : release IP address for the specified interface\n");
    fprintf( stderr, "\t/iponly : set IP address only for the specified interface\n");
    fprintf( stderr, "\t/wait   : wait for the IP address to be assigned up to maximum secs\n");
    fprintf( stderr, "\t/q      : quit kipcfg daemon\n");
    fprintf( stderr, "\n");
}

static void parse_options( int argc, char *argv[], struct options *opts )
{
    int need_ifname;
    int need_secs;
    int invalid;
    int i;

    if( !opts )
        exit( 1 );

    memset( opts, 0, sizeof( *opts ));

    need_ifname = 0;
    need_secs   = 0;
    for( i = 1; i < argc; i++ )
    {
        invalid = 0;

        if( need_ifname )
        {
            if( !strncmp( argv[ i ], "lan", 3 ) && strlen( argv[ i ]) == 4 )
            {
                opts->ifnum = atoi( argv[ i ] + 3 );
            }
            else
                invalid = 1;

            need_ifname = 0;
        }
        else if( need_secs )
        {
            int secs = atoi( argv[ i ]);

            if( secs )
            {
                if( secs < 1 )
                    secs = 1;

                if( secs > 60 )
                    secs = 60;

                opts->wait = secs;
            }
            else
                invalid = 1;

            need_secs = 0;
        }
        else if( !stricmp( argv[ i ], "/req") || !stricmp( argv[ i ], "-req"))
        {
            opts->mode = MODE_REQUEST;

            need_ifname = 1;
        }
        else if( !stricmp( argv[ i ], "/rel") || !stricmp( argv[ i ], "-rel"))
        {
            opts->mode = MODE_RELEASE;

            need_ifname = 1;
        }
        else if( !stricmp( argv[ i ], "/iponly") || !stricmp( argv[ i ], "/iponly"))
        {
            opts->iponly = 1;
        }
        else if( !stricmp( argv[ i ], "/wait") || !stricmp( argv[ i ], "-wait"))
        {
            need_secs = 1;
        }
        else if( !stricmp( argv[ i ], "/q") || !stricmp( argv[ i ], "-q"))
        {
            opts->quit = 1;
        }
        else
            invalid = 1;

        if( i + 1 == argc && need_ifname )
        {
            fprintf( stderr, "Missing interface!!!\n" );
            show_usage();

            exit( 1 );
        }

        if( i + 1 == argc && need_secs )
        {
            fprintf( stderr, "Missing maximum wait time!!!\n");
            show_usage();

            exit( 1 );
        }

        if( invalid )
        {
            fprintf( stderr, "Invalid argument : %s\n", argv[ i ]);
            show_usage();

            exit( 1 );
        }
    }
}

int main( int argc, char *argv[])
{
    struct options    opts;
    struct daemon_msg dm;

    if( argc == 2 && !strcmp( argv[ 1 ], DHCPC_DAEMON_MAGIC ))
        return daemon_main();

    if( !daemon_alive())
        daemon_start( argv[ 0 ]);

    fprintf( stdout, "kipcfg %s - Very Simple DHCP Client\n", KIPCFG_VER );

    if( argc < 2 )
    {
        show_usage();

        return 1;
    }

    parse_options( argc, argv, &opts );

    if( opts.quit )
    {
        fprintf( stdout, "Please wait to quit daemon...\n");
        dm.msg  = DCDM_QUIT;
        dm.wait = opts.wait;
        daemon_call( &dm );
        if( dm.msg == DCDE_NO_ERROR )
            fprintf( stdout, "Terminated daemon successfully.\n");
        else
            fprintf( stderr, "Failed to terminate daemon!!!, rc = %ld\n", dm.arg );

        return 0;
    }

    if( opts.mode == MODE_REQUEST )
    {
        struct if_sem *sem;

        if( opts.wait )
        {
            sem = daemon_if_sem_open( opts.ifnum );
            if( sem )
            {
                fprintf( stderr, "Configuring IP address for interface lan%d is still progressing...\n",
                         opts.ifnum );
                fprintf( stderr, "Please wait to finish it\n");

                daemon_if_sem_close( sem );

                return 0;
            }

            sem = daemon_if_sem_create( opts.ifnum );
        }

        dm.msg    = DCDM_REQUEST;
        dm.arg    = opts.ifnum;
        dm.iponly = opts.iponly;
        dm.wait   = opts.wait;
        daemon_call( &dm );

        if( opts.wait )
        {
            if( daemon_if_sem_wait( sem, opts.wait ) < 0 )
            {
                fprintf( stdout, "Configuring IP address for interface lan%d is progressing...\n",
                         opts.ifnum );
            }
            else if( dm.msg == DCDE_IP_ALREADY_ASSIGNED )
            {
                fprintf( stderr, "IP adress for interface lan%d was already configured.\n",
                         opts.ifnum );
                fprintf( stderr, "Release IP adress using /rel option first\n");
            }
            else
            {
                fprintf( stdout, "Configured IP address for interface lan%d successfully.\n",
                         opts.ifnum );
            }

            daemon_if_sem_close( sem );

            return 0;
        }

        // check return codes for no-wait mode
        switch( dm.msg )
        {
            case DCDE_NO_ERROR :
                fprintf( stdout, "Configuring IP address for interface lan%d is progressing...\n",
                         opts.ifnum );
                break;

            case DCDE_PIPE_ERROR :
                fprintf( stderr, "Pipe error occured!!! rc = %ld. Maybe kipcfg daemon quitted\n",
                         dm.arg );
                break;

            case DCDE_INVALID_INTERFACE :
                fprintf( stderr, "Invalid interface lan%d\n", opts.ifnum );
                    break;

            case DCDE_IP_ALREADY_ASSIGNED :
                fprintf( stderr, "IP adress for interface lan%d was already configured.\n",
                         opts.ifnum );
                fprintf( stderr, "Release IP adress using /rel option first\n");
                break;

            case DCDE_IP_ASSIGNING :
                fprintf( stderr, "Configuring IP address for interface lan%d is still progressing...\n",
                         opts.ifnum );
                fprintf( stderr, "Please wait to finish it\n");
                break;

            case DCDE_IP_RELEASING :
                fprintf( stderr, "Releasing IP address for interface lan%d is progressing...\n",
                         opts.ifnum );
                fprintf( stderr, "Please wait to finish it\n");
                break;

            case DCDE_SOCKET_ERROR :
                fprintf( stderr, "Socket error occured\n");
                break;
        }
    }
    else
    {
        dm.msg  = DCDM_RELEASE;
        dm.arg  = opts.ifnum;
        dm.wait = opts.wait;
        daemon_call( &dm );

        switch( dm.msg )
        {
            case DCDE_NO_ERROR :
                fprintf( stdout, "Released IP address for interface lan%d successfully\n",
                         opts.ifnum );
                break;

            case DCDE_PIPE_ERROR :
                fprintf( stderr, "Pipe error occured!!! rc = %ld. Maybe kipcfg daemon quitted\n",
                         dm.arg );
                break;

            case DCDE_INVALID_INTERFACE :
                fprintf( stderr, "Invalid interface lan%d\n", opts.ifnum );
                break;

            case DCDE_IP_NOT_ASSIGNED :
                fprintf( stderr, "Interface lan%d is not under control of kipcfg\n",
                         opts.ifnum );
                fprintf( stderr, "Configure IP adress using /req option first\n");
                break;

            case DCDE_IP_ASSIGNING :
                fprintf( stderr, "Configuring IP address for interface lan%d is still progressing...\n",
                         opts.ifnum );
                fprintf( stderr, "Please wait to finish it first\n");
                break;

            case DCDE_IP_RELEASING :
                fprintf( stderr, "Releasing IP address for interface lan%d is already progressing...\n",
                         opts.ifnum );
                fprintf( stderr, "Please wait to finish it\n");
                break;
        }
    }

    return 0;
}
