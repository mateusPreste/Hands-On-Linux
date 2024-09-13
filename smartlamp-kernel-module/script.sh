#!/bin/bash

# Function to remove the kernel module
remove_module() {
    echo "Removing kernel module..."
    sudo rmmod serial.ko
    exit 0
}

# Trap Ctrl+C and call the remove_module function
trap remove_module SIGINT

# Run make
make

# Insert the kernel module with sudo
sudo insmod serial.ko

# Watch the kernel message buffer with sudo
sudo dmesg -w | grep SmartLamp
