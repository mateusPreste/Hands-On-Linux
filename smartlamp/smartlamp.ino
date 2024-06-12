// Defina os pinos de LED e LDR
// Defina uma variável com valor máximo do LDR (4000)
// Defina uma variável para guardar o valor atual do LED (10)
#include "DHT.h"
#define DHT_PIN 21
#define DHTTYPE DHT22
DHT dht(DHT_PIN, DHTTYPE);

int ledPin = 19;
int ledValue;

int ldrPin = 4;
// Faça testes no sensor ldr para encontrar o valor maximo e atribua a variável ldrMax
int ldrMax = 4000;

void setup() {
    Serial.begin(9600);
    pinMode(ledPin, OUTPUT);
    pinMode(ldrPin, INPUT);
    pinMode(DHT_PIN, INPUT);
    
    Serial.printf("SmartLamp Initialized.\n");
    Serial.setTimeout(10);
    dht.begin();
    ledUpdate(10);
    processCommand(String("GET_LDR"));
}
// Função loop será executada infinitamente pelo ESP32
void loop() {
    //Obtenha os comandos enviados pela serial 
    //e processe-os com a função processCommand
    if (Serial.available() > 0) {
      String str = Serial.readString();
      str.trim();
      processCommand(str);
    }
}
void processCommand(String command) {
    // compare o comando com os comandos possíveis e execute a ação correspondente      
    if (command.equals(String("GET_LDR"))) {
      Serial.printf("RES GET_LDR %d\n", ldrGetValue());
    } else if (command.equals(String("GET_LED"))) {
       Serial.printf("RES GET_LED %d\n", ledValue);
    } else if (command.substring(0, 7).equals(String("SET_LED"))) {
      int set_led_value = command.substring(7).toInt();  
      if ( set_led_value >= 0 && set_led_value <=100) {
        ledUpdate(set_led_value);
        Serial.printf("RES SET_LED 1\n");
      } else {
        Serial.printf("RES SET_LED -1\n");
      }
    } else if (command.equals(String("GET_TEMP"))) {
      Serial.print("RES GET_TEMP ");
      Serial.print(GetTemperature());
      Serial.print("\n");
    } else if (command.equals(String("GET_HUM"))) {
      Serial.print("RES GET_HUM ");
      Serial.print(GetHumidity());
      Serial.print("\n");
    } else {
      Serial.printf("ERR Unknown command.");
    }
}
// Função para atualizar o valor do LED
void ledUpdate(int value) {
    // Valor deve convertar o valor recebido pelo comando SET_LED para 0 e 255
    // Normalize o valor do LED antes de enviar para a porta correspondente
    int led_value_norm = map(value, 0, 100, 0, 255);
    ledValue = value;
    analogWrite(ledPin, led_value_norm);
}
// Função para ler o valor do LDR
int ldrGetValue() {
    // Leia o sensor LDR e retorne o valor normalizado entre 0 e 100
    // faça testes para encontrar o valor maximo do ldr (exemplo: aponte a lanterna do celular para o sensor)       
    // Atribua o valor para a variável ldrMax e utilize esse valor para a normalização
    return map(analogRead(ldrPin), 0, ldrMax, 0, 100);
    
}

// Função para ler o valor de temperatura
int GetTemperature() {
    return dht.readTemperature();
} 

// Função para ler o valor de humidade
int GetHumidity() {
    return dht.readHumidity();
}
