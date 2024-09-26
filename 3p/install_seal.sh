#!/bin/bash

pwd
clone_dir=$(pwd)/clone
install_dir=$(pwd)/seal

echo "Clone directory: $clone_dir"
echo "Installation directory: $install_dir"
echo ""

# if the directory does not exist, create it
if [ ! -d "$clone_dir" ]; then
  mkdir $clone_dir
fi

# change to the directory
cd $clone_dir

# check if SEAL directory does not exist
if [ ! -d "SEAL" ]; then
	# clone SEAL
	echo "Cloning SEAL"
	git -c http.sslVerify=false clone --depth 1 --branch 4.1.2 https://github.com/Microsoft/SEAL
	echo ""
fi

# change to the SEAL directory
cd SEAL

if [ ! -d $install_dir ]; then
	# build SEAL
	echo "Building and installing SEAL"
	cmake -S . -B build -DCMAKE_INSTALL_PREFIX=$install_dir \
		-DCMAKE_BUILD_TYPE=Release -DSEAL_BUILD_EXAMPLES=OFF -DSEAL_BUILD_TESTS=ON -DSEAL_BUILD_BENCH=ON \
		-DSEAL_BUILD_DEPS=ON -DSEAL_USE_INTEL_HEXL=ON -DSEAL_USE_MSGSL=ON -DSEAL_USE_ZLIB=ON -DSEAL_USE_ZSTD=ON \
		-DBUILD_SHARED_LIBS=OFF -DSEAL_BUILD_SEAL_C=OFF -DSEAL_USE_CXX17=ON -DSEAL_USE_INTRIN=ON
	cmake --build build
	cmake --install build
	echo ""
fi

# test the installation
echo "Testing the installation"
cd $clone_dir/SEAL/native/tests
sed -i 's/4.1.1/4.1.2/g' CMakeLists.txt # fix CMakeList.txt
cmake -S . -B build -DSEAL_ROOT=$install_dir
cmake --build build
cd build/bin
./sealtest
echo ""

# performance test
echo "Running performance test"
cd $clone_dir/SEAL/native/bench
sed -i 's/4.1.1/4.1.2/g' CMakeLists.txt # fix CMakeList.txt
cmake -S . -B build -DSEAL_ROOT=$install_dir
cmake --build build
cd build/bin
./sealbench
echo ""