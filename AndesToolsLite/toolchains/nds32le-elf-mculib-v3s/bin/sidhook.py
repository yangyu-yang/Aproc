#!/usr/bin/python
# -*- coding: UTF-8 -*- 

import os, sys, struct, re
import time
import getopt
import configparser


INTERFACE_TEMPLATE = (
"AndeSim v1.10.1889 (b183ea1)\n"
"Build Date: 2018/06/04 11:29\n"
"SID 0.1-net\n"
"Copyright 1999-2002 Red Hat, Inc.\n"
"sid is free software, covered by the GNU General Public License,\n"
"as interpreted by the COPYING.SID terms.\n"
)

sid_pro = "C:\\Andestech\\AndeSight_RDS_v323\\toolchains\\nds32le-elf-mculib-v3s\\bin\\nds32le-elf-gdb.exe"
params_c = ""

if __name__=="__main__":
    params = sys.argv[1:]
    # print("hook ok!")
    out_c = open('D:\\hook.txt', 'a+')
    out_c.write("\n----------------start new-----------------------\n")
    for item in params:
        # print(item)
        params_c += item + ' '
        out_c.write(item + '\n')
        # if item == '--version':
        #     print(INTERFACE_TEMPLATE)
    
    sid_pro += ' ' + params_c
    r_v = os.system(sid_pro)
    # print("---------------------------------------")
    out_c.write("\n---------------------------------------\n")
    print(r_v)
    # print(params_c)
    # print("finished")

    out_c.close()