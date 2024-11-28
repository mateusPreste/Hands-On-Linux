#!/bin/bash

# Função para verificar se um módulo está carregado
is_module_loaded() {
    # Verifica se o módulo está carregado
    lsmod | awk '{print $1}' | grep -q "^$1$"
}

# Remover o módulo cp210x se estiver carregado
if is_module_loaded "cp210x"; then
    echo "Removendo módulo cp210x..."
    sudo rmmod cp210x
else
    echo "Módulo cp210x não está carregado."
fi

# Inserir o módulo serial se não estiver carregado
if ! is_module_loaded "serial"; then
    echo "Inserindo módulo serial..."
    sudo insmod serial.ko
else
    echo "Módulo serial já está carregado."
    echo "Removendo módulo serial..."
    sudo rmmod serial

    # Após remover o módulo serial, inserir o módulo cp210x
    echo "Inserindo módulo cp210x..."
    sudo modprobe cp210x
fi
