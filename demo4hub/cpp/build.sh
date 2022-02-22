mkdir -p build
cd build
cmake ../
make -j4


cp ../hub_struct.h hub_interface.h ../../../lib/

cp libdemo4hub.so  ../../../lib/

ls -al ../../../lib/
