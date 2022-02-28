# !/bin/sh

mkdir -p build
cd build/
cmake ../
make -j4

cp -fr ../../etc ./

cp -fr ../../script/*.sh ./

cat start.sh

sleep 2s

./start.sh
