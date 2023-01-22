/*
 * Copyright © 2008-2014 Stéphane Raimbault <stephane.raimbault@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define LOG_TAG "test"

#include <stdio.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <modbus.h>
#include "agent_core_lib.h"
#include "duration_dist.h"
#include "path.h"
#include "iniparser.h"

enum {
    TCP,
    RTU
};

typedef enum{
    COIL,
    REGISTER
} reg_type_t;

#ifndef DEFAULT_TEST_SIZE
#define DEFAULT_TEST_SIZE 8
#endif

void help(const char* bin)
{
    printf("\nModbus client to measure data bandwith\n");
    printf("Usage:\n  %s -c [coil test data size] -r [register test data size] -p [tcp port]\n\n", bin);
}

#ifndef MAX_CLIENTS 
#define MAX_CLIENTS 100
#endif

int main(int argc, char *argv[])
{
    int s = -1;
    modbus_t *ctx = NULL;
    modbus_mapping_t *mb_mapping = NULL;
    int rc;
    int use_backend = TCP;
    char path[256];
    int coil_test_size = DEFAULT_TEST_SIZE;
    int reg_test_size = DEFAULT_TEST_SIZE;
    int debug_flags = 0;
    int tcp_port = 1502;
    int stats_duration = 30;
    int master_socket = 0;
    int sockets[MAX_CLIENTS] = {0};
    /* Add the server socket */
    path_init(argv[0]);

    log_init("modbus_test_server", 
        get_log_path(),
        get_local_config_path(path, sizeof(path),(char*)"logcfg.ini"));

    snprintf(path, sizeof(path), "%s/mb_test_stats.log", get_log_path());

    dictionary  *   ini = NULL;
    snprintf(path, sizeof(path), "%s/set.ini", get_bin_path());
    if(access(path, F_OK) == 0 && (ini = iniparser_load(path)))
    {
        printf("loaded ini file: %s", path);
        coil_test_size = iniparser_getint(ini, ":coil_test_size", DEFAULT_TEST_SIZE);
        reg_test_size = iniparser_getint(ini, ":reg_test_size", DEFAULT_TEST_SIZE);
        debug_flags = iniparser_getint(ini, ":modbus_debug", 0);
        tcp_port = iniparser_getint(ini, ":port", 1502);
        stats_duration = iniparser_getint(ini, ":stats_duration", 30);

        printf("loaded ini: \ncoil_test_size=%d, \nreg_test_size=%d, \nmodbus_debug=%d, \nport=%d\nstats_duration=%d\n",
            coil_test_size, reg_test_size, debug_flags, tcp_port, stats_duration);
        WARNING("loaded ini: \ncoil_test_size=%d, \nreg_test_size=%d, \nmodbus_debug=%d, \nport=%d\nstats_duration=%d\n",
            coil_test_size, reg_test_size, debug_flags, tcp_port, stats_duration);
    }
    else
    {
        printf("To use the configuration, add a file set.ini under the binary path\n");
    }
    
    int opt;
    while ((opt = getopt(argc, argv, "r:c:p:h")) != -1)
    {
        switch (opt)
        {
        case 'r'://pidfile
            reg_test_size = atoi(optarg);
            break;
        case 'c'://pidfile
            coil_test_size = atoi(optarg);
            break;
        case 'p'://pidfile
            tcp_port = atoi(optarg);
            WARNING("cmd: tcp_port=%d", tcp_port);
            break;
        case 'h'://pidfile
            help(argv[0]);
            return 0;
        default:
            help(argv[0]);
            return 0;
        }
    }

    if(coil_test_size < 0 || coil_test_size > 800)
    {
        help(argv[0]);
        printf("The max coil test size is 800. current: %d\n", coil_test_size);
        exit(1);
    }

    if(reg_test_size < 0 || reg_test_size > 56)
    {
        help(argv[0]);
        printf("The max reg test size is 56. current: %d\n", reg_test_size);
        exit(1);
    }

    printf("Using reg_test_size=%d, coil_test_size=%d,stats_duration=%d\n",reg_test_size,  coil_test_size, stats_duration);
    WARNING("Using reg_test_size=%d, coil_test_size=%d,stats_duration=%d\n",reg_test_size,  coil_test_size, stats_duration);

    if (use_backend == TCP) {
        printf("started TCP mode, now listen on port %d\n", tcp_port);
        WARNING("started TCP mode, now listen on port %d\n", tcp_port);
        ctx = modbus_new_tcp("0.0.0.0", tcp_port);
        master_socket = modbus_tcp_listen(ctx, 1);
        //sockets[0] = modbus_tcp_accept(ctx, &master_socket);

    } else {
        printf("started RTU mode. Opening /dev/ttyUSB0 with 115200 baud rate.\n");
        ctx = modbus_new_rtu("/dev/ttyUSB0", 115200, 'N', 8, 1);
        modbus_set_slave(ctx, 1);
        modbus_connect(ctx);
    }

    modbus_set_debug(ctx, debug_flags);

    //printf("connected.\n");
    //WARNING("connected");

    mb_mapping = modbus_mapping_new(MODBUS_MAX_READ_BITS, MODBUS_MAX_READ_BITS,
                                    MODBUS_MAX_READ_REGISTERS, MODBUS_MAX_READ_REGISTERS);
    if (mb_mapping == NULL) {
        fprintf(stderr, "Failed to allocate the mapping: %s\n",
                modbus_strerror(errno));
        WARNING("Failed to allocate the mapping: %s\n",modbus_strerror(errno));
        WARNING("Exiting..");
        modbus_free(ctx);
        return -1;
    }
    unsigned char c_coil = 0, c_reg = 0;
    tick_time_t last_set_us = 0;
    tick_time_t last_coil_set_us = 0;
    snprintf(path, sizeof(path), "%s/mb_test_stats.log", get_bin_path());
    duration_counter_t counter_c = ws_create_duration_counter(DEFAULT_DURATION_MGR, "coil_cycle (us)", 1000);
    duration_counter_t counter_r = ws_create_duration_counter(DEFAULT_DURATION_MGR, "reg_cycle (us)", 1000);
    ws_start_duration_report(DEFAULT_DURATION_MGR, stats_duration, path);

    fd_set refset;
    FD_ZERO(&refset);
    FD_SET(master_socket, &refset);
    int fdmax = master_socket;
    while(1)
    {
        
        fd_set rdset;
        FD_ZERO(&rdset);
       
        rdset = refset;

        if (select(fdmax+1, &rdset, NULL, NULL, NULL) == -1) 
        {
            perror("Server select() failure.");
            break;
        }

        if (FD_ISSET(master_socket, &rdset)) 
        {
            /* A client is asking a new connection */
            socklen_t addrlen;
            struct sockaddr_in clientaddr;
            int newfd;

            /* Handle new connections */
            addrlen = sizeof(clientaddr);
            memset(&clientaddr, 0, sizeof(clientaddr));
            newfd = accept(master_socket, (struct sockaddr *)&clientaddr, &addrlen);
            if (newfd == -1) 
            {
                perror("Server accept() error");
            } 
            else 
            {
                if(fdmax < newfd) fdmax = newfd;
                FD_SET(newfd, &refset);
                for (int i = 0; i < MAX_CLIENTS; i++) 
                {
                    if(sockets[i] == 0)
                    {
                        sockets[i] = newfd;
                        printf("New connection from %s:%d on socket %d, #%d\n", inet_ntoa(clientaddr.sin_addr), clientaddr.sin_port, newfd, i);
                        WARNING("New connection from %s:%d on socket %d, #%d", inet_ntoa(clientaddr.sin_addr), clientaddr.sin_port, newfd, i);
                        break;
                    }
                }
            }
            continue;
        }

        /* Run through the existing connections looking for data to be
         * read */
        for (int i = 0; i < MAX_CLIENTS; i++) 
        {
            if (sockets[i] == 0 || !FD_ISSET(sockets[i], &rdset)) 
            {
                continue;
            }

            modbus_set_socket(ctx, sockets[i]);
            uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];
            rc = modbus_receive(ctx, query);
            if (rc > 0) {
                
                modbus_reply(ctx, query, rc, mb_mapping);
                int offset;
                int slave;
                int function;
                uint16_t address;

                offset = modbus_get_header_length(ctx);
                slave = query[offset - 1];
                function = query[offset];
                address = (query[offset + 1] << 8) + query[offset + 2];
                TraceV(0x20,"recieved a message. bytes: %d", rc);
                TraceV(0x10,"slave: %d, function:%d, address=%d", slave, function,address);

                if(function == 6 || function == 16)
                {
                    if(reg_test_size > 0)
                    {
                        int test_size = reg_test_size * 2;
                        uint8_t * p = (uint8_t *)mb_mapping->tab_registers;
                        int i = 0;
                        for(i=0; i< reg_test_size * 2;i++)
                        {
                            if(p[test_size+i] != c_reg)
                                break;
                        }
                        if(i == test_size || last_set_us == 0)
                        {
                            tick_time_t now = bh_get_tick_us();
                            if(last_set_us != 0)
                            {
                                ws_count_duration(counter_r, now-last_set_us);
                            }
                            c_reg ++;
                            memset(mb_mapping->tab_registers, c_reg, test_size);
                            last_set_us  = now;
                            TraceV(1, "set the reg read zone to [%d]\n", c_reg);
                        }
                        else
                        {
                            TraceV(4, "wrote the reg, but not match the output");
                        }
                    }
                    
                }
                else if(function == 5 || function == 15)
                {
                    if(coil_test_size > 0)
                    {
                        int i = 0;
                        for(i=0; i< coil_test_size;i++)
                        {
                            if(mb_mapping->tab_bits[coil_test_size+i] != c_coil)
                                break;
                        }
                        if(i == coil_test_size || last_coil_set_us == 0)
                        {
                            tick_time_t now = bh_get_tick_us();
                            if(last_coil_set_us != 0)
                            {
                                ws_count_duration(counter_c, now-last_coil_set_us);
                            }
                            c_coil = (c_coil+1)%2;
                            memset(mb_mapping->tab_bits, c_coil, coil_test_size);
                            last_coil_set_us = now;
                            TraceV(1, "set the coil read zone to [%d]\n", c_coil);
                        }
                        else
                        {
                            TraceV(4, "wrote the coils, but not match the output");
                        }
                    }
                }
            } else if (rc  == -1) {
                /* Connection closed by the client or error */
                printf("Connection closed on socket #%d\n", i);
                WARNING("Connection closed on socket #%d. %s", i, modbus_strerror(errno));

                close(sockets[i]);
                /* Remove from reference set */
                FD_CLR(sockets[i], &refset);
                sockets[i] = 0;
            }
        }
    }

    printf("Quit the loop: %s\n", modbus_strerror(errno));
    WARNING("Quit the loop: %s", modbus_strerror(errno));

    modbus_mapping_free(mb_mapping);
    if (s != -1) {
        close(s);
    }
    /* For RTU, skipped by TCP (no TCP connect) */
    modbus_close(ctx);
    modbus_free(ctx);

    return 0;
}
