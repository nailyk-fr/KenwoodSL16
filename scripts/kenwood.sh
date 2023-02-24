#!/bin/bash


serial=/dev/ttyUSB0

stty -F $serial speed 115200 cs8

echo -n "$1" > $serial

read line < $serial
echo "read: $line"
read line < $serial
echo "read: $line"

