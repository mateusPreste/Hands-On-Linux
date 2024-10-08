// Defina os pinos de LED e LDR
// Defina uma variável com valor máximo do LDR (4000)
// Defina uma variável para guardar o valor atual do LED (10)
int ledPin = 18;
int ledValue = 10;

int ldrPin = 4;
// Faça testes no sensor ldr para encontrar o valor maximo e atribua a variável ldrMax
int ldrMax = 4000;

void setup() {
    Serial.begin(9600);
    
    pinMode(ledPin, OUTPUT);
    pinMode(ldrPin, INPUT);
    
    Serial.println("SmartLamp Initialized.");
    // delay(2000);
    // processCommand(String("GET_LDR"));
    // Serial.println(analogRead(ldrPin));


}

// Função loop será executada infinitamente pelo ESP32
void loop() {
    //Obtenha os comandos enviados pela serial 
    //e processe-os com a função processCommand

    if (Serial.available() > 0) {
      String str = Serial.readString();
      str.trim();
      processCommand(str);
      // delay(1000);
      // processCommand(String("GET_LDR"));
    }


}


void processCommand(String command) {
    // compare o comando com os comandos possíveis e execute a ação correspondente     
    Serial.print("command :");
    Serial.println(command);

    if (command.equals(String("GET_LDR"))){
        Serial.printf("RES GET_LDR %d\n", ldrGetValue());
    }
    else if (command.equals(String("GET_LED"))){
        Serial.printf("RES GET_LED %d\n", ledValue);
    }
    else if (command.substring(0, 7).equals(String("SET_LED"))){
        int valor_led = command.substring(7).toInt();
        if (valor_led >= 0 && valor_led <= 100){
            ledUpdate(valor_led); // atualizar led 
            Serial.printf("RES SET_LED 1\n");
        }
        else{
            Serial.printf("RES SET_LED -1\n");
        }
    }
}

// Função para atualizar o valor do LED
void ledUpdate(int valor) {
    // Valor deve convertar o valor recebido pelo comando SET_LED para 0 e 255
    // Normalize o valor do LED antes de enviar para a porta correspondente
    
    int valor_led_norm = map(valor, 0, 100, 0, 255);
    ledValue = valor;
    analogWrite(ledPin, valor_led_norm);

}   

// Função para ler o valor do LDR
int ldrGetValue() {
    // Leia o sensor LDR e retorne o valor normalizado entre 0 e 100
    // faça testes para encontrar o valor maximo do ldr (exemplo: aponte a lanterna do celular para o sensor)       
    // Atribua o valor para a variável ldrMax e utilize esse valor para a normalização
    
    return map(analogRead(ldrPin), 0, ldrMax, 0, 100);

}