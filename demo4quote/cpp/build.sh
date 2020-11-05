mkdir -p cmake/build
pushd cmake/build
cmake ../..
make -j 4
cp ../../config.json ./
./demo4quote
popd
