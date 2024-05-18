// Defina os pinos de LED e LDR
// Defina uma variável com valor máximo do LDR (4000)
// Defina uma variável para guardar o valor atual do LED (10)
int ledPin = 23;
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
    String cmd = Serial.readString();

    //e processe-os com a função processCommand
    processCommand(cmd);
}


void processCommand(String command) {
    // compare o comando com os comandos possíveis e execute a ação correspondente      
    command.trim();

    if (command.startsWith("SET_LED")) {
        int pos = command.indexOf(' ');
        if (pos != -1) {
        String intensityString = command.substring(pos + 1);
        int intensity = intensityString.toInt();
        ledUpdate(intensity);
    }
    } else if (command.equals("GET_LED")) {
        ledGetValue();
    }
    else if (command.equals("GET_LDR")) {
        ldrGetValue();
    }
}

// Função para atualizar o valor do LED
void ledUpdate(int led_intensity) {
    if(led_intensity >= 0 && led_intensity <= 100) {
        ledValue = led_intensity;
        analogWrite(ledPin, ledValue);
        Serial.println("RES SET_LED 1");
    } else {
        Serial.println("RES SET_LED -1");
    }
}

// Função para ler o valor do LDR
int ldrGetValue() {
    // Leia o sensor LDR e retorne o valor normalizado.
    int value=analogRead(ldrPin);
    if(value >= ldrMax){
      return 255;
    }
    return value*255/ldrMax;
}

// Função para ler o valor do LED
int ledGetValue() {
    Serial.printf("RES GET_LED ");
    Serial.println(ledValue);
}
