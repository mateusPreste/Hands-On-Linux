#!/bin/bash

# Function to remove the kernel module
remove_module() {
    echo "Removing kernel module..."
    sudo rmmod serial.ko
    echo "Cleaning..."
    make clean
    exit 0
}

# # Trap Ctrl+C and call the remove_module function
# trap remove_module SIGINT

# Run make
make

# Insert the kernel module with sudo
sudo insmod serial.ko

# Watch the kernel message buffer with sudo
sudo dmesg -w 
# sudo dmesg -w | grep SmartLamp

remove_module
