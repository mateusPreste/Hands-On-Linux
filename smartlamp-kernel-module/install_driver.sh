sudo rmmod serial
sudo rmmod sysfs
sudo rmmod usbserial

sudo rmmod cp210x
make
sudo rmmod sysfs.ko
sudo insmod sysfs.ko
lsmod | grep sysfs
