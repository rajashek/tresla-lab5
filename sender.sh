#!/bin/bash
make clean; make
date1=$(date +"%s")
echo $date1
./fscp -s -h $1 -f $2 -ack 3 > /dev/null && ./fscp -r > /dev/null 
date2=$(date +"%s")
echo $date2
diff=$(($date2-$date1))
echo "$(($diff / 60)) minutes and $(($diff % 60)) seconds elapsed."
throughput=$(($((2 * 1048576000 * 8))/$(($(($(($diff / 60 )) * 60 ))+$(($diff%60))))))
throughput_mbps=$(($throughput/1000000))
echo "Throughput achieved in Mbps"
echo "$throughput_mbps Mbps"
echo "comparison results "
echo cmp $2 input.bin
 
