#!/bin/bash

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <read|write>"
    exit 1
fi

mode=$1

if [ "$mode" == "write" ]; then
    while true; do
        echo "Hello, Dummy Device!" | sudo dd of=/dev/dummy_chardev bs=256 count=1
        sleep 1
    done
elif [ "$mode" == "read" ]; then
    while true; do
        sudo dd if=/dev/dummy_chardev bs=256 count=1 | hexdump -C
        sleep 1
    done
else
    echo "Invalid mode: $mode. Use 'read' or 'write'."
    exit 1
fi
