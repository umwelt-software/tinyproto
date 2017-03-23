#!/bin/sh

rm channel*.txt
if [ "$1" = "" ]; then
    make examples
    if [ $? -ne 0 ]; then
       exit 1
    fi
    ./bld/testproto 2> 1.txt
fi

cat 1.txt | grep 0x604740 | grep IN  > channel1_in.txt
cat 1.txt | grep 0x604740 | grep OUT  > channel1_out.txt
cat 1.txt | grep 0x604758 | grep OUT  > channel2_out.txt
cat 1.txt | grep 0x604758 | grep IN  > channel2_in.txt

