/**
 * @file smartlamp.ino
 * @author Mateus (to do)
 * @author Lahis Almeida (lahis.gomes.almeida@gmail.com)
 * @author Nelson (to do)
 * @author Itala (to do)
 * @author Wanderson  (to do)
 * 
 * @brief Main code of ESP32 smartlamp firmware. This code is responsible for (... to do)
 * 
 * @version 1.0
 * @date 2024-09-11
 * 
 * @copyright Copyright (c) 2024
 * 
 */


// --- Definação de variáveis
int ledPin = 5;
int ledValue = 0;

int ldrPin = 36;
int ldrValue = 10;
int ldrMax = 1000;

// Função setup de configuração
void setup() {
    Serial.begin(115200);
    
    pinMode(ledPin, OUTPUT);
    pinMode(ldrPin, INPUT);
    
    delay(2000);
    Serial.printf("SmartLamp Initialized.\n");

    // Uncomment line bellow to calibrate LDR max value
    // calibrate_ldrMax();

}

// Função loop será executada infinitamente pelo ESP32
void loop() {

    // Fica a espera de comandos seriais 
    while (Serial.available() == 0)
    {     
      // Le o comando até que o timeout padrão (to do) seja esgotado 
      String command = Serial.readString();  
      // Serial.println(command);

      processCommand(command);
      // delay(100);
    }
    
}

// // Função responsável por calibrar o valor máximo do LDR
// void calibrate_ldrMax()
// {
//     int value = analogRead(ldrPin);  // read the input pin
//     Serial.println(value);
//     delay(1000);
// }

// Função responsável por processar comandos
void processCommand(String command) 
{
    // Remove qualquer eventual caracter \r\n no final do comando serial
    String driver_command = command;
    driver_command.trim();  
    
    // Condições que comparam comandos pré-estabelecidos e executam ações correspondentes
    if (driver_command.indexOf("SET_LED") == 0) // encontrou o comando na serial
    {
      int index = driver_command.indexOf("SET_LED");
      ledValue = driver_command.substring(index + 8).toInt();
      if (ledValue >= 0 && ledValue <= 100)
      {
        ledUpdate(ledValue);
        Serial.printf("RES SET_LED %d\r\n", ledValue);  
      }
      else
      {
        Serial.printf("RES SET_LED %d\r\n", -1);  
      }
    }
    else if (driver_command == "GET_LED") 
    {
      Serial.printf("RES GET_LED %d\r\n", ledValue);  
    }
    else if (driver_command == "GET_LDR")
    {
      ldrValue = ldrGetValue();
      Serial.printf("RES GET_LDR  %d\r\n", ldrValue);  
    }
    else
    {
      Serial.println("ERR Unknown command");
    }   
}

// Função para atualizar o valor do LED
void ledUpdate(int ledValue) {
    // Valor deve convertar o valor recebido pelo comando SET_LED para 0 e 255
    // Normalize o valor do LED antes de enviar para a porta correspondente
    int ledValueNormalized = map(ledValue, 0, 100, 0, 255);
    analogWrite(ledPin, ledValueNormalized);
}

// Função para ler o valor do LDR
int ldrGetValue() {
    // Leia o sensor LDR e retorne o valor normalizado entre 0 e 100
    // faça testes para encontrar o valor maximo do ldr (exemplo: aponte a lanterna do celular para o sensor)       
    // Atribua o valor para a variável ldrMax e utilize esse valor para a normalização
    return 100;
}
