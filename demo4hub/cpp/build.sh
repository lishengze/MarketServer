mkdir -p cmake/build
pushd cmake/build
cmake ../..
make -j
popd
cp cmake/build/libdemo4hub.so ../../lib/
cp hub_struct.h ../../lib/
cp hub_interface.h ../../lib/
cp ../../base/cpp/basic.h ../../lib/
cp ../../base/cpp/decimal.h ../../lib/
cp ../../base/cpp/quote.h ../../lib/
