#!/bin/bash

# setup environment
export CPU=x86_64
export OS=linux
export VARIANT=debug
export AJ_ROOT=$HOME/src/core-alljoyn/build/$OS/$CPU/$VARIANT/dist/cpp
export AJ_LIB=$AJ_ROOT/lib
export AJ_INC=$AJ_ROOT/inc
export LD_LIBRARY_PATH=$AJ_LIB:$LD_LIBRARY_PATH
export MB_LIB=/usr/local/lib
export MB_INC=/usr/local/include
export LD_LIBRARY_PATH=$MB_LIB:$LD_LIBRARY_PATH
export BST_LIB=/usr/local/lib
export BST_INC=$HOME/src/boost_1_66_0
export LD_LIBRARY_PATH=$BST_LIB:$LD_LIBRARY_PATH

# build
export SRC=dcs
make -C ../build

# run
./../build/bin/debug/$SRC -c /home/tylor/dev/powerlab/DCS/data/config.ini -o y
