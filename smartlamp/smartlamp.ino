// Defina os pinos de LED e LDR
// Defina uma variável com valor máximo do LDR (4000)
// Defina uma variável para guardar o valor atual do LED (10)
int ledPin = 39;
int ledValue = 0;

int ldrPin = 36;
// Faça testes no sensor ldr para encontrar o valor maximo e atribua a variável ldrMax
int ldrMax = 1000;

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
    Serial.println("Enter data:");
    while (Serial.available() == 0) {     //wait for data available
      String teststr = Serial.readString();  //read until timeout
      teststr.trim();                        // remove any \r \n whitespace at the end of the String
      if (teststr == "red") {
        Serial.println("A primary color");
      } else {
        Serial.println("Something else");
      }
    }
    
}


void processCommand(String command) {
    // compare o comando com os comandos possíveis e execute a ação correspondente      
}

// Função para atualizar o valor do LED
void ledUpdate() {
    // Valor deve convertar o valor recebido pelo comando SET_LED para 0 e 255
    // Normalize o valor do LED antes de enviar para a porta correspondente
}

// Função para ler o valor do LDR
int ldrGetValue() {
    // Leia o sensor LDR e retorne o valor normalizado entre 0 e 100
    // faça testes para encontrar o valor maximo do ldr (exemplo: aponte a lanterna do celular para o sensor)       
    // Atribua o valor para a variável ldrMax e utilize esse valor para a normalização
}
