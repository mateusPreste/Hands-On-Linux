#!/bin/bash

rmmod cp210x
# Defina o nome do módulo que você deseja remover e reinserir
MODULE_NAME="serial"
MODULE_FILE="${MODULE_NAME}.ko"

# Verifique se o módulo está carregado
if lsmod | grep "$MODULE_NAME" &> /dev/null; then
    echo "Removendo o módulo $MODULE_NAME..."
    sudo rmmod "$MODULE_NAME"
    if [ $? -ne 0 ]; then
        echo "Falha ao remover o módulo $MODULE_NAME. Saindo..."
        exit 1
    fi
else
    echo "Módulo $MODULE_NAME não está carregado."
fi

# Execute o make
echo "Executando 'make'..."
make
if [ $? -ne 0 ]; then
    echo "Falha na execução do 'make'. Saindo..."
    exit 1
fi

# Insira novamente o módulo
echo "Inserindo novamente o módulo $MODULE_NAME..."
sudo insmod "$MODULE_FILE"
if [ $? -ne 0 ]; then
    echo "Falha ao inserir o módulo $MODULE_NAME. Saindo..."
    exit 1
fi

echo "Módulo $MODULE_NAME reinserido com sucesso."
