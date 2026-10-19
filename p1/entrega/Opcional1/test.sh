#!/bin/sh
sudo rmmod modlist.ko
make EXTRA_CFLAGS=-DPARTE_OPCIONAL
sudo insmod modlist.ko
echo add hola > /proc/modlist
echo add lol > /proc/modlist
