#/bin/bash

rm -rf build
mkdir build
source env.sh
cd build
cmake -DPICO_BOARD=pico_w ..
make -j4

if [ $? -ne 0 ]; then
  echo "Build failed"
  exit 1
fi
picotool load -f ./mp3player.uf2