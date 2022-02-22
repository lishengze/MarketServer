mkdir -p /build
cd build/
cmake ../
make -j4

cp ../config.json ./
cp ../../shell/*.sh ./

./start.sh

popd
