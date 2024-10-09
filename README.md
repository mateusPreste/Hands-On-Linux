

# DevTITANS 05 - HandsOn Linux - Equipe 0X

Bem-vindo ao repositório da Equipe 0X do HandsON de Linux do DevTITANS! Este projeto contém um firmware para o ESP32 escrito em formato Arduino `.ino`, bem como um driver do kernel Linux escrito em C. O objetivo é demonstrar como criar uma solução completa de hardware e software que integra um dispositivo ESP32 com um sistema Linux.

## Tabela de Conteúdos

- [DevTITANS 05 - HandsOn Linux - Equipe 0X](#devtitans-05---handson-linux---equipe-0x)
  - [Tabela de Conteúdos](#tabela-de-conteúdos)
  - [Contribuidores](#contribuidores)
  - [Introdução](#introdução)
  - [Recursos](#recursos)
  - [Requisitos](#requisitos)
  - [Configuração de Hardware](#configuração-de-hardware)
  - [Instalação](#instalação)
    - [Firmware ESP32](#firmware-esp32)
    - [Driver Linux](#driver-linux)
  - [Uso](#uso)

  - [Contato](#contato)


## Contribuidores
<img src="https://github.com/user-attachments/assets/830648f1-1dd6-443b-83ef-7be8c4b237dd" width="180" >
<img src="https://github.com/user-attachments/assets/c65d095c-7b32-48a2-90f1-0053934c7da2" width="180" >
<img src="https://github.com/user-attachments/assets/5a5fa6a2-f8dc-47b5-bf17-df98449addf0" width="180" >
<img src="https://github.com/user-attachments/assets/d197148a-0e62-4c30-94b5-e6261f90ae71" width="180" >
<img src="https://github.com/user-attachments/assets/5d9a4c44-9c71-4542-9972-fa673815b108" width="180" >


- **Mateus Pantoja 01:** Desenvolvedor do Firmware e Mantenedor do Projeto
- **Lahis Almeida 02:** Desenvolvedor do Firmware
- **Nelson Villarreal 03:** Desenvolvedor do Driver Linux
- **Wanderson Lima Ferreira 04:** Desenvolvedor do Driver Linux
- **Itala Menezes 05:** Desenvolvedor do Firmware e Escritor da Documentação

## Introdução

Este projeto serve como um exemplo para desenvolvedores interessados em construir e integrar soluções de hardware personalizadas com sistemas Linux. Inclui os seguintes componentes:
- Firmware para o microcontrolador ESP32 para lidar com operações específicas do dispositivo.
- Um driver do kernel Linux que se comunica com o dispositivo ESP32, permitindo troca de dados e controle.

## Recursos

- **Firmware ESP32:**
  - Aquisição básica de dados de sensores.
  - Comunicação via Serial com o driver Linux.
  
- **Driver do Kernel Linux:**
  - Rotinas de inicialização e limpeza.
  - Operações de arquivo de dispositivo (`GET_LED`, `SET_LED`, `GET_LDR`).
  - Comunicação com o ESP32 via Serial.

## Requisitos

- **Hardware:**
  - Placa de Desenvolvimento ESP32
  - Máquina Linux
  - Protoboard e Cabos Jumper
  - Sensor LDR
  
- **Software:**
  - Arduino IDE
  - Kernel Linux 4.0 ou superior
  - GCC 4.8 ou superior
  - Make 3.81 ou superior

## Configuração de Hardware

1. **Conecte o ESP32 à sua Máquina Linux:**
    - Use um cabo USB.
    - Conecte os sensores ao ESP32 conforme especificado no firmware.

2. **Garanta a alimentação e conexões adequadas:**
    - Use um protoboard e cabos jumper para montar o circuito.
    - Consulte o diagrama esquemático fornecido no diretório `esp32` para conexões detalhadas.

## Instalação

### Firmware ESP32

1. **Abra o Arduino IDE e carregue o firmware:**
    ```sh
    Arquivo -> Abrir -> Selecione `smartlamp.ino`
    ```

2. **Configure a Placa e a Porta:**
    ```sh
    Ferramentas -> Placa -> Node32s
    Ferramentas -> Porta -> Selecione a porta apropriada
    ```

3. **Carregue o Firmware:**
    ```sh
    Sketch -> Upload (Ctrl+U)
    ```

### Driver Linux

1. **Clone o Repositório:**
    ```sh
    git clone https://github.com/seuusuario/Hands-On-Linux.git
    cd Hands-On-Linux
    ```

2. **Compile o Driver:**
    ```sh
    cd smartlamp-kernel-module
    make
    ```

3. **Carregue o Driver:**
    ```sh
    sudo insmod smartlamp.ko
    ```

4. **Verifique o Driver:**
    ```sh
    dmesg | tail
    ```

## Uso

Depois que o driver e o firmware estiverem configurados, você poderá interagir com o dispositivo ESP32 através do sistema Linux.

- **Escrever para o Dispositivo:**
    ```sh
    echo "GET_LED" > /sys/kernel/smartlamp/led
    ```

- **Ler do Dispositivo:**
    ```sh
    cat /sys/kernel/smartlamp/led
    ```

- **Verificar Mensagens do Driver:**
    ```sh
    dmesg | tail
    ```

- **Remover o Driver:**
    ```sh
    sudo rmmod smartlamp
    ```
    
## Contato

Para perguntas, sugestões ou feedback, entre em contato com o mantenedor do projeto em [maintainer@example.com](mailto:maintainer@example.com).
