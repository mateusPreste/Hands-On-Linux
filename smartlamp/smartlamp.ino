// Defina os pinos de LED e LDR
// Defina uma variável com valor máximo do LDR (4000)
// Defina uma variável para guardar o valor atual do LED (10)
int ledPin = 22;
int ledValue = 10;

int ldrPin = 36;
int ldrMax = 4000;

void setup() {
    Serial.begin(9600);
    
    pinMode(ledPin, OUTPUT);
    pinMode(ldrPin, INPUT);
    
    Serial.printf("SmartLamp Initialized.\n");


    
}

// Função loop será executada infinitamente pelo ESP32
void loop() {
    //Obtenha os comandos enviados pela serial 
    //e processe-os com a função processCommand
    // Check if data is available to read
    if (Serial.available() > 0) {
      // Read the incoming byte
      char incomingByte = Serial.read();

      // Print the incoming byte to the Serial Monitor
      Serial.print("Received: ");
      Serial.println(incomingByte);
    }

    // You can add your logic here based on the received data
}


void processCommand(String command) {
    // compare o comando com os comandos possíveis e execute a ação correspondente      
}

// Função para atualizar o valor do LED
void ledUpdate() {
    // Normalize o valor do LED antes de enviar para a porta correspondente
}

// Função para ler o valor do LDR
int ldrGetValue() {
    // Leia o sensor LDR e retorne o valor normalizado.
}
