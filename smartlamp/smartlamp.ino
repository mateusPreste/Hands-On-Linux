/**
 * @file smartlamp.ino
 * @author Mateus Pantoja
 * @author Lahis Almeida (lahis.gomes.almeida@gmail.com)
 * @author Nelson (to do)
 * @author Pedro Mendes (to do)
 * @author Wanderson  (to do)
 * 
 * @brief Main code of ESP32 smartlamp firmware. This code is responsible for (... to do)
 * 
 * @version 1.0
 * @date 2024-10-08
 * 
 * @copyright Copyright (c) 2024
 * 
 */
// --- Definação de includes
 #include "DHT.h"

 // --- Definação de defines
#define DHTTYPE DHT11 
#define DHTPIN 14

// --- Definação de variáveis
int ledPin = 26;
int ledValue = 0;

int ldrPin = 27;
int ldrValue = 10;
int ldrMax = 4045;


DHT dht(DHTPIN, DHTTYPE);

// Função setup de configuração
void setup() {
    Serial.begin(9600);

    pinMode(ledPin, OUTPUT);
    pinMode(ldrPin, INPUT);


    analogWrite(ledPin, 0);
    dht.begin();
  
    
//    Serial.printf("SmartLamp Initialized.\n");
    processCommand("GET_TEMP");


    // Uncomment line bellow to recalibrate LDR max value
    // calibrate_ldrMax();
    //Chamando o processo command para realizar tarefa  3. Ao final da tarefa apenas comentar a linha 



}

// Função loop será executada infinitamente pelo ESP32
void loop() {
    delay(2000);

    // Fica a espera de comandos seriais 
    while (Serial.available() > 0)
    {     
      // Lê o comando até que o timeout padrão (to do) seja esgotado 
      String command = Serial.readString();  
      processCommand(command);
      //delay(2000);
    }
        
}

// Função responsável por processar comandos
void processCommand(String command) 
{
    // Remove qualquer eventual caracter \r\n no final do comando serial
    String driver_command = command;
    driver_command.trim();  
    
    // ---- Condições que comparam comandos pré-estabelecidos e executam ações correspondentes
    // Checa se o comando SET_LED foi recebido na serial
    if (driver_command.indexOf("SET_LED") == 0) 
    {
      int index = driver_command.indexOf("SET_LED");
      ledValue = driver_command.substring(index + 8).toInt();
      if (ledValue >= 0 && ledValue <= 100)
      {
        ledUpdate(ledValue);
        Serial.printf("RES SET_LED %d\r\n", 1);  
      }
      else
      {
        Serial.printf("RES SET_LED %d\r\n", -1);  
      }
    }
    // Checa se o comando GET_LED foi recebido na serial
    else if (driver_command == "GET_LED") 
    {
      Serial.printf("RES GET_LED %d\r\n", ledValue);  
    }
    // Checa se o comando GET_LDR foi recebido na serial
    else if (driver_command == "GET_LDR")
    {
      ldrValue = ldrGetValue();
      Serial.printf("RES GET_LDR %d\r\n", ldrValue);  
    }

    // Checa se o comando GET_TEMP foi recebido na serial
    else if (driver_command == "GET_TEMP")
    {
      float tempValue = tempGetValue();
      Serial.printf("RES GET_TEMP %.2f\r\n", tempValue);  
    }
    // Checa se o comando GET_TEMP foi recebido na serial
    else if (driver_command == "GET_HUM")
    {
      float humiValue = humiGetValue();
      Serial.printf("RES GET_HUM %.2f\r\n", humiValue);  
    }
    // Checa se o comando qualquer outro comando diferente dos pre-estabelecidos foi recebido na serial
    else
    {
      Serial.println("ERR Unknown command");
    }   
}

// Função para atualizar o valor do LED
void ledUpdate(int ledValue) {
    // Normalizeação do valor do LED antes de envio para a porta correspondente
    int ledValueNormalized = map(ledValue, 0, 100, 0, 255);

    // Envio de valor normazliado para a porta correspondente
    analogWrite(ledPin, ledValueNormalized); //ledValueNormalized

  delay(50);

}

// Função para ler o valor do LDR
int ldrGetValue() {
    // Leitura do sensor LDR
    int value = analogRead(ldrPin); 
    //Serial.printf("LDR sensor value: %d\n", value);

    // Normalização do valor do sensor LDR para a faixa de 0 a 100
    int ldrNormalizedValue = map(value, 0, 4045, 0, 100);
    return ldrNormalizedValue;
}


// Função para ler o valor do DHT Temperatura
float tempGetValue() {
    // Leitura do sensor DHT Temperatura
    float temp = dht.readTemperature();
    //Serial.printf("TEMP sensor value: %.2f\n", temp);
    return temp;
}

// Função para ler o valor do DHT Humidity
float humiGetValue() {
    // Leitura do sensor DHT Humidity
   
    float humi = dht.readHumidity(); 
    //Serial.printf("HUMI sensor value: %d\n", humi);

    return humi;
}

// Função responsável por calibrar o valor máximo do LDR
void calibrate_ldrMax()
{   
  // O Loop abaixo é utilizado para encontrar o valor maximo do LDR ao
  // se apontar, por exemplo, a lanterna do celular para o sensor)       
  while (true)
  {
    // Leitura de valor do sensor LDR
    int value = analogRead(ldrPin); 
    Serial.printf("LDR sensor value: %d\n", value);
    delay(500);
  }

}
