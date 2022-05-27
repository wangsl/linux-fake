#!/bin/bash

mac="aa:bb:cc:dd:ee:ff"
interfaces=$(netstat -i | awk '{print $1}' | egrep -v "^Kernel$|^Iface$|^ib0|^lo$|^br-|^docker")

FAKE_MAC=
for name in $interfaces; do
  FAKE_MAC="${FAKE_MAC};${name}=${mac}"
done
export FAKE_MAC

export LD_PRELOAD="./libfakemac.so"

ifconfig
