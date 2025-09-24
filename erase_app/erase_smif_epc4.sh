#!/bin/sh

source ./common.sh

${OPENOCD} \
        -c "set SMIF_BANKS { 0 {addr 0x70000000 size 0x4000000} 1 {addr 0x60000000 size 0x1000000} }" \
        -c "adapter serial ${CM33_AP_SN}" \
        -c "bindto 0.0.0.0; gdb_port 3333" \
        -c "source [find interface/kitprog3.cfg]" \
        -c "transport select swd" \
        -c "source [find target/infineon/pse84xgxs4.cfg]" \
        -c "gdb_breakpoint_override hard" \
        -c "init" \
        -c "reset init" \
        -c "echo Erasing_<SMIF>; flash erase_address 0x60000000 0x1000000"
