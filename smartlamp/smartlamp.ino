// Defina os pinos de LED e LDR
// Defina uma variável com valor máximo do LDR (4000)
// Defina uma variável para guardar o valor atual do LED (10)
int ledPin = 26;
int ledValue = 10;

int ldrPin = 39;
// Faça testes no sensor ldr para encontrar o valor maximo e atribua a variável ldrMax
int ldrMax = 4048;

void setup() {
  Serial.begin(115200);

  pinMode(ledPin, OUTPUT);
  //analogWrite(ledPin,200);
  pinMode(ldrPin, INPUT);

  Serial.printf("SmartLamp Initialized.\n");



}

// Função loop será executada infinitamente pelo ESP32
void loop() {
  //Obtenha os comandos enviados pela serial
  //e processe-os com a função processCommand
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    processCommand(command);
  }

}


void processCommand(String command) {
  // compare o comando com os comandos possíveis e execute a ação correspondente
  int spaceIndex = command.indexOf(' ');
  String cmd = command.substring(0, spaceIndex);
  String param = command.substring(spaceIndex + 1);

  int new_cmd = 0;
  if (cmd == "SET_LED") {
    int value = param.toInt();
    if (value >= 0 && value <= 100) {
      ledValue = value;
      ledUpdate(value);
      Serial.println("RES SET_LED 1");
    } else
      Serial.println("RES SET_LED -1");
  } else if (cmd == "GET_LED")
    Serial.printf("RES GET_LED %s", String(ledValue));
  else if (cmd == "GET_LDR") {
    int ldrValue = ldrGetValue();
    Serial.printf("RES GET_LDR %s", String(ldrValue));
  } else
    Serial.println("ERR Unknown command.");

}

// Função para atualizar o valor do LED
void ledUpdate(int value) {
  // Valor deve convertar o valor recebido pelo comando SET_LED para 0 e 255
  // Normalize o valor do LED antes de enviar para a porta correspondente
  Serial.println(value);
    analogWrite(ledPin, map(value, 0, 100, 0, 255));
  //analogWrite(ledPin, value);
}

// Função para ler o valor do LDR
int ldrGetValue() {
  // Leia o sensor LDR e retorne o valor normalizado entre 0 e 100
  // faça testes para encontrar o valor maximo do ldr (exemplo: aponte a lanterna do celular para o sensor)
  // Atribua o valor para a variável ldrMax e utilize esse valor para a normalização
  int sensorValue = analogRead(ldrPin);
  return map (sensorValue, 0, ldrMax, 0, 100);
}
