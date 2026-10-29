#!/bin/bash
while true; do
    for n in {0..7}; do
        echo $n >/dev/led
        sleep 0.2
    done
done
