#!/bin/bash

i=0
while [ $i -le 50000 ]
do
    echo abcdefghij >> a.txt
    i=$((i+1))
done
