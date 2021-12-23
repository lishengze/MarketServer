# !/bin/bash

mkdir -p ../../lib/comm

cp comm.h comm_declare.h comm_interface_define.h comm_type_def.h comm_log.h ../../lib/comm

cp build/libbcts_comm.so ../../lib/comm

ls -al ../../lib/comm
