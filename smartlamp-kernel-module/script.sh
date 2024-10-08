#!/bin/bash

module="smartlamp.ko"

# Function to remove the kernel module
remove_module() {
    echo "Removing kernel module..."
    sudo rmmod $module 2>/dev/null || echo "Module not loaded."
    echo "Cleaning..."
    make clean
}

# Variable to hold the background process ID
dmesg_pid=0

# Function to stop the background process of dmesg
stop_dmesg() {
    if [[ $dmesg_pid -ne 0 ]]; then
        echo "Parando monitoramento de logs do SmartLamp (PID: $dmesg_pid)..."
        kill $dmesg_pid
        wait $dmesg_pid 2>/dev/null
        dmesg_pid=0
    fi
}

# Trap Ctrl+C to call cleanup functions
trap "remove_module; stop_dmesg; exit 0" SIGINT

while true; do
    # Display menu
    echo "Select an option:"
    echo "1 - Iniciar os processos"
    echo "2 - Executar lsusb -t"
    echo "3 - Remover o módulo e parar monitoramento"
    echo "4 - Monitorar logs do SmartLamp"
    echo "5 - Sair"

    read -p "Digite a opção desejada: " option

    case $option in
        1)
            # Inicia os processos
            make

            echo "Removing kernel module: cp210x"
            # Remove the Arduino module with sudo, ignoring errors if not loaded
            sudo rmmod cp210x 2>/dev/null || echo "Module cp210x not loaded."

            echo "Installing kernel module: {$module}" 
            # Insert the kernel module with sudo
            time sudo insmod $module
            ;;

        2)
            # Executa o comando lsusb -t
            echo "Executing lsusb -t"
            lsusb -t
            ;;

        3)
            # Remove o módulo chamando a função
            remove_module
            stop_dmesg
            ;;

        4)
            # Executa o comando para monitorar logs do SmartLamp em segundo plano
            if [[ $dmesg_pid -eq 0 ]]; then
                echo "Monitorando logs do SmartLamp em segundo plano..."
                sudo dmesg -w &
                dmesg_pid=$!  # Armazena o PID do processo de segundo plano
            else
                echo "Monitoramento de logs já está em execução."
            fi
            ;;

        5)
            # Para o monitoramento de logs e remove o módulo antes de sair
            remove_module
            stop_dmesg
            echo "Saindo..."
            sudo modprobe cp210x
            exit 0
            ;;

        *)
            echo "Opção inválida. Tente novamente..."
            ;;
    esac

    # Pausa para aguardar uma nova entrada
    echo ""
done
