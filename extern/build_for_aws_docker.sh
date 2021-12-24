# Using dockerimages from https://github.com/aws/aws-lambda-base-images
yum -y groupinstall "Development Tools"
yum -y install openssl openssl-devel libcurl libcurl-devel gcc72 gcc72-c++ python36-devel

curl -O https://cmake.org/files/v3.18/cmake-3.18.0.tar.gz
tar -xvf cmake-3.18.0.tar.gz
cd cmake-3.18.0
./bootstrap
make -j6 install
cd ..

curl -LO https://boostorg.jfrog.io/artifactory/main/release/1.77.0/source/boost_1_77_0.tar.gz
tar -xvf boost_1_77_0.tar.gz
cd boost_1_77_0
./bootstrap.sh --with-python=python3
./b2 cxxflags="-fPIC" link=static install
cd ..

git clone --recurse-submodules https://github.com/aws/aws-sdk-cpp.git
cd aws-sdk-cpp
mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_ONLY="s3" -DBUILD_SHARED_LIBS="off" ..
make -j6 install
cd ../..

git clone https://github.com/redis/hiredis.git
cd hiredis/
make install

# Now, can run cmake in the python directory, and build fmi.so, which can be distributed in a Lambda layer