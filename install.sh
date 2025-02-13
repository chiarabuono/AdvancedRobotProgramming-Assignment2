#!/bin/sh

sudo apt-get update
sudo apt-get upgrade
sudo apt-get install --fix-missing

sudo apt install terminator
sudo apt install konsole
sudo apt install libncurses-dev
sudo apt install libcjson-dev

curl -o fastdds.tgz 'https://www.eprosima.com/component/ars/item/eProsima_Fast-DDS-v3.1.0-Linux.tgz?format=tgz&category_id=7&release_id=169&Itemid=0'

mkdir fastdds
tar -xvzf ./fastdds.tgz -C ./fastdds
cd fastdds
sudo ./install.sh

make clean
make 
