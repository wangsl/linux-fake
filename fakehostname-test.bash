#!/bin/bash

export LD_PRELOAD="./libfakehostname.so"
export FAKE_HOSTNAME="xx.yy.zz"

hostname
uname -a
