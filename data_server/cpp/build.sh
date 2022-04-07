mkdir -p build
cd build/
cmake ../
make -j4

cp -fr ../../etc ../

if [[ $1 == "-qa" ]]; then
  cp -fr ../../script/qa/*.sh ./
elif [[ $1 == "-prd" ]]; then
  cp -fr ../../script/prd/*.sh ./
elif [[ $1 == "-stg" ]]; then
  cp -fr ../../script/stg/*.sh ./
else
  cp -fr ../../script/dev/*.sh ./  
fi

cat start.sh

./clean.sh

./start.sh

