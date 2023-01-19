#!/usr/bin/env python3

# -*- coding: utf-8 -*-
import sys
import time
import random
import os
import json
import argparse

bus_name='1502'

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description = "Modbus test server validator")
    parser.add_argument('-c', dest = 'coil_size', action = 'store',
        default = 0,
        help = 'The size of coils for test')
    parser.add_argument('-r', dest = 'reg_size', action = 'store',
        default = 0,
        help = 'The size of registers for test')
    parser.add_argument('-s', dest = 'step', action = 'store_true',
        default = False,
        help = 'Wait input for each round')    
    args = parser.parse_args()

    dirname, filename = os.path.split(os.path.abspath(sys.argv[0]))
    repo_root_dir = os.path.abspath(os.path.join(dirname, "../../../.."))
    sys.path.append(repo_root_dir + "/sdk/external/CoAPthon3/coapthon")

    from coapthon.client.helperclient import HelperClient
    from coapthon import defines

    test_coil = True
    test_size = 8
    if(int(args.coil_size) == 0 and int(args.reg_size) > 0):
        test_coil = False
        test_size = int(args.reg_size)
    elif int(args.coil_size) > 0:
        test_size = int(args.coil_size)


    client = HelperClient(server=("127.0.0.1", 5683))

    fc_read = 3
    fc_write = 6
    if test_coil:
        fc_read = 1
        fc_write = 5

    cnt = 0;
    while True:
        path = "/mb/{}/1/0?fc={}&items={}".format(bus_name,fc_read, test_size)
        response = client.get(path);
        if (response is None or  response.payload is None):
            print ("Read fail")
            time.sleep(1)
            continue
        try:    
            j_result = json.loads(response.payload)
        except:
            print(f'recieved data not JSON: {response.payload}')
            continue

        path = "/mb/{}/1/{}?fc={}&items={}".format(bus_name, test_size, fc_write, test_size)
        
        client.put(path, (defines.Content_types["application/json"],json.dumps(j_result)))
        cnt = cnt + 1

        if args.step:
            print(f'recieved data: {response.payload}')
            input(f'total: {cnt}, continue?')