# Altere esta linha para o arquivo da task que voce quer compilar.
#
# Exemplos:
#   Tarefa 2.1: obj-m += probe_2025.o
#   Tarefa 2.2: obj-m += serial_write_2025.o
#   Tarefa 2.3: obj-m += serial_read_2025.o
#   Tarefa 2.4: obj-m += sysfs_2025.o
#   Tarefa 3.1: obj-m += test_driver.o
obj-m += test_driver.o
PWD := $(CURDIR)

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
