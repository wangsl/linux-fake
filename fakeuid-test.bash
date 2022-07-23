#!/bin/bash

id sw77
id

export LD_PRELOAD="./libfakeuid.so"

export FAKE_UID=1296493
export FAKE_EUID=1296493
export FAKE_GID=1296493 
export FAKE_EGID=1296493

id

