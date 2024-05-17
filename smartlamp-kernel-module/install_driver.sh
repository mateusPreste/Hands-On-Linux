sudo rmmod serial
sudo rmmod usbserial

sudo rmmod cp210x
make
sudo rmmod serial.ko
sudo insmod serial.ko
lsmod | grep serial
