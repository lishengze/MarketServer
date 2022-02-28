mkdir -p build
cd build/
cmake ../
make -j4

cp -fr ../../etc ./

cp ../../script/*.sh ./

./start.sh
