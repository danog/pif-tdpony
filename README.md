# PIF-TDPony

This is a PHP interface for [tdlib](https://core.telegram.org).

# Installation


```
# Installing PHP
sudo apt-get install python-software-properties software-properties-common
sudo LC_ALL=C.UTF-8 add-apt-repository ppa:ondrej/php
sudo apt-get update
sudo apt-get install php7.2 php7.2-dev php7.2-fpm php7.2-curl php7.2-xml php7.2-gmp php7.1-zip -y

# Installing tdlib
sudo apt-get install gperf cmake clang ninja-build libssl-dev
git clone https://github.com/tdlib/td
cd td
mkdir build
cd build
cmake -GNinja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX:PATH=/usr ..
sudo cmake --build . --target install
cd ../..

# Installing PHP-CPP
git clone https://github.com/CopernicaMarketingSoftware/PHP-CPP
cd PHP-CPP
make -j$(nproc)
sudo make install
cd ..

# Installing PIF-TDPony
git clone https://github.com/danog/pif-tdpony
cd pif-tdpony
mkdir build
cd build
cmake ..
sudo cmake --build . --target install
cd ../../
```


Usage:


