#!/bin/bash
while true; do
    for n in {0..9} {a..f}; do
        echo $n >/dev/display7s
        sleep 0.2
    done
done
