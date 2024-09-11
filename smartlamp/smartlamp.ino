// Definação de variáveis
int ledPin = 5;
int ledValue = 0;

int ldrPin = 36;
int ldrValue = 10;
int ldrMax = 1000;

void setup() {
    Serial.begin(115200);
    
    pinMode(ledPin, OUTPUT);
    pinMode(ldrPin, INPUT);
    
    Serial.printf("SmartLamp Initialized.\n");
    delay(2000);
}

// Função loop será executada infinitamente pelo ESP32
void loop() {
    //Obtenha os comandos enviados pela serial 
    //e processe-os com a função processCommand

    while (Serial.available() == 0)
    {     // wait for data available
      String command = Serial.readString();  //read until timeout
      processCommand(command);
      //delay(100);
    }
    
}


void processCommand(String command) {
    // compare o comando com os comandos possíveis e execute a ação correspondente     
    String driver_command = command;
    driver_command.trim();  // remove any \r \n whitespace at the end of the String
    
    if (driver_command.indexOf("SET_LED") == 0) // encontrou o comando na serial
    {
      int index = driver_command.indexOf("SET_LED");
      ledValue = driver_command.substring(8).toInt();
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
