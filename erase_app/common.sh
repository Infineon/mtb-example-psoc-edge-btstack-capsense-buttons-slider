#!/bin/sh

# Access ports
CM33_AP_SN=18160B8900212400 

if [ -z "${CM33_AP_SN}" ]; then
    echo "ERROR: CM33_AP_SN is not set";
    exit 1
fi

# Openocd file path
OPENOCD="C:/Infineon/Tools/ModusToolboxProgtools-1.5/openocd/bin/openocd"
