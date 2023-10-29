#!/bin/bash

git clone https://github.com/mit-pdos/xv6-public.git
rm -rf xv6-public/.git
apt install qemu
cd xv6-public
make