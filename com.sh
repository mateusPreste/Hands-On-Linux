#!/bin/bash

comando=$1
argumento=$2

case $comando in 
    "GET_LED")
        cat /sys/kernel/smartlamp/led
        ;;
    "GET_LDR")
        cat /sys/kernel/smartlamp/ldr
        ;;
    "SET_LED")
        if [ -n "$argumento" ]; then
            echo $argumento | sudo tee -a /sys/kernel/smartlamp/led
        else
            echo "ERROR: Argumento esperado para SET_LED"
        fi
        ;;
    *)
        echo "Comando desconhecido: $comando"
        ;;
esac

# Para executar o script, é necessário dar permissão de execução:
# sudo chmod +x ./com.sh