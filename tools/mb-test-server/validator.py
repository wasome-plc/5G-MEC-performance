#!/usr/bin/env python3

# -*- coding: utf-8 -*-
import sys
import time
import random
import os
import json
import argparse
import modbus_tk
import modbus_tk.defines as cst
from modbus_tk import modbus_tcp
import logging

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
    test_coil = True
    test_size = 8
    fc_read = cst.READ_COILS
    fc_write = cst.WRITE_MULTIPLE_COILS
    if(int(args.coil_size) == 0 and int(args.reg_size) > 0):
        test_coil = False
        test_size = int(args.reg_size)
        fc_read = cst.READ_HOLDING_REGISTERS
        fc_write = cst.WRITE_MULTIPLE_REGISTERS;
        
    elif int(args.coil_size) > 0:
        test_size = int(args.coil_size)
        

    if test_coil:
        print ("test coil: size = " + str(test_size))
    else:
        print ("test reg: size = " + str(test_size))

    logger = modbus_tk.utils.create_logger("console", level=logging.DEBUG)
    try:

        # Connect to the slave
        master = modbus_tcp.TcpMaster('127.0.0.1', 1502)
        master.set_timeout(5.0)
        
    except modbus_tk.modbus.ModbusError as exc:
        logger.error("%s- Code=%d", exc, exc.get_exception_code())
        sys.exit("connect fail")

    logger.info("connected")

    while True:
        try:
            result = master.execute(1,fc_read, 0, test_size)
            # logger.info(result)
            master.execute(1,fc_write, test_size, test_size, output_value = result)
        except modbus_tk.modbus.ModbusError as exc:
            logger.error("%s- Code=%d", exc, exc.get_exception_code())
        except Exception as exc:
            logger.error("Except:%s", exc)
            break
        # time.sleep(0.1)
        if args.step:
            print(f'recieved data: {result}')
            input(f'continue?')