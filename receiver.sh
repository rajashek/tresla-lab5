#!/bin/bash
make clean; make
./fscp -r 
./fscp -s -h $1 -f input.bin -ack 3 > /dev/null
