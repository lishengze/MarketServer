#!/bin/bash

mkdir -p cmake/build
pushd cmake/build
cmake ../..
make -j
cp ../../config.json ./
./demo4quote
popd
