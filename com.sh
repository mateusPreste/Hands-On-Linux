#!/bin/bash

comando=$1
argumento=$2

sudo chmod 777 /sys/kernel/smartlamp/led
sudo chmod 777 /sys/kernel/smartlamp/ldr
case $comando in 
    "get_led")
        cat /sys/kernel/smartlamp/led
        ;;
    "get_ldr")
        cat /sys/kernel/smartlamp/ldr
        ;;
    "get_temp")
        cat /sys/kernel/smartlamp/temp
        ;;
    "get_hum")
        cat /sys/kernel/smartlamp/hum
        ;;
    "set_led")
        if [ -n "$argumento" ]; then
            echo $argumento | sudo tee /sys/kernel/smartlamp/led 
        else
            echo "ERROR: Argumento esperado para SET_LED"
        fi
        ;;
    "get_ldr_all")
        while true; do
            cat /sys/kernel/smartlamp/ldr
            sleep 0.8
        done
        ;;
    *)
        echo "Comando desconhecido: $comando"
        ;;
esac

# Para executar o script, é necessário dar permissão de execução:
# sudo chmod +x ./com.sh