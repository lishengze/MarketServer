mkdir -p build
cd build/
cmake ../
make -j4

cp ../config.json ./
cp ../../script/*.sh ./

./start.sh

popd
