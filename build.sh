#/bin/bash

rm -rf build
mkdir build
source env.sh
cd build
cmake -DPICO_BOARD=pico_w ..
make -j4