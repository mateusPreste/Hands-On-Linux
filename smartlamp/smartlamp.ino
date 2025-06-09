/*
  Autora: Jakeline Gimaque de Mesquita
  Descrição: Firmware para lampada inteligente referente ao Hands-On de Linux do DevTITANS
  Data: 13/09/2024
*/

// Pinos de LED e LDR
int ledPin = 26;
int ldrPin = 39;

// Variável com valor máximo do LDR definido após teste
int ldrMax = 4048;

// Variável para guardar o valor atual do LED (valor inicial de 10)
int ledValue = 10;



//Função setup define os pinos como de entrada ou saída
void setup() {
  Serial.begin(9600);

  pinMode(ledPin, OUTPUT);
  pinMode(ldrPin, INPUT);

  Serial.printf("SmartLamp Initialized.\n");
}



// Função loop será executada infinitamente pelo ESP32
void loop() {
  //Obtem os comandos enviados pela serial e processa-os com a função processCommand
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    processCommand(command);
  }

}



// Compara o recebido no loop com os comandos possíveis e executa a ação correspondente
void processCommand(String command) {
  int spaceIndex = command.indexOf(' ');
  String cmd = command.substring(0, spaceIndex); //Variavel cmd armazenará o comando inserido
  String param = command.substring(spaceIndex + 1); //Variavel param armazenará o numero que acompanha a entrada SET_LED


  //Ajusta a intensidade da luz com base em um intervalo de 0 a 100 
  if (cmd == "SET_LED") {
    ledValue = param.toInt();
    if (ledValue >= 0 && ledValue <= 100) {
      ledUpdate(ledValue);
      Serial.print("RES SET_LED 1\n");
    } else {
      ledValue = 10; //volta para o valor inicial
      Serial.print("RES SET_LED -1\n");
    }

  }


  //Imprime a intensidade da luz na escala de 0 a 100
  else if (cmd == "GET_LED")
    Serial.printf("RES GET_LED %s\n", String(ledValue));


  //Imprime o valor do LDR na escala de 0 a 100
  else if (cmd == "GET_LDR") {
    int ldrValue = ldrGetValue();
    Serial.printf("RES GET_LDR %s\n", String(ldrValue));
  }
  
  //Imprime um erro indicando que o comando digitado não foi reconhecido
  else
    Serial.println("ERR Unknown command.");

}



// Função para atualizar o valor do LED
//Converte o valor recebido pelo comando SET_LED que esta na escala 0/100 para a escala de operação do Led
//Quando o valor esta normalizado é enviado ao led pela porta logica
void ledUpdate(int value) {
  analogWrite(ledPin, map(value, 0, 100, 0, 255));

}

// Função para ler o valor do LDR
int ldrGetValue() {
  int sensorValue = analogRead(ldrPin); //Ler o sensor LDR
  return map (sensorValue, 0, ldrMax, 0, 100); //Retorna o valor do LDR na escala 0/100
}
