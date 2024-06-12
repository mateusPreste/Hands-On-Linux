// Defina os pinos de LED e LDR
// Defina uma variável com valor máximo do LDR (4000)
// Defina uma variável para guardar o valor atual do LED (10)
#include <DHT.h>
#include <DHT_U.h>


/*
   https://gist.github.com/aferreira44/6bbe8fab792e24afc6c1389da714aa99
   https://embarcados.com.br/como-programar-o-esp32-na-arduino-ide/
   https://kb.deec.uc.pt/books/deec/page/programa-led-a-piscar
   https://portal.vidadesilicio.com.br/led-com-esp32-curso-esp32-basico/
   https://portal.vidadesilicio.com.br/como-utilizar-o-led-rgb-com-arduino/
   https://www.paulotrentin.com.br/programacao/dicas/lendo-uma-string-com-arduino-via-serial/
   https://www.youtube.com/watch?v=GDxwDsphyuk&ab_channel=BrincandocomIdeias
   https://blogmasterwalkershop.com.br/arduino/arduino-exibindo-e-lendo-dados-da-serial
   https://forum.arduino.cc/t/test-if-a-string-contains-a-string/478927/4
   https://www.blogdarobotica.com/2022/06/30/controlando-o-brilho-do-led-usando-potenciometro-e-arduino/
   https://www.tutorialspoint.com/find-if-a-substring-exists-within-a-string-in-arduino#:~:text=In%20order%20to%20check%20if,searching%20for%20within%20another%20string.
   https://forum.arduino.cc/t/duvida-com-substring/1045145
   https://www.clubedohardware.com.br/forums/topic/1360900-como-utilizar-o-parseint-arduino/
   sudo chmod a+rw /dev/ttyUSB0

*/

int PINO_DHT = 26;

int PINO_LED = 23;
int PINO_LDR = 33;
int ledValue = 10;
int ldrMax;
DHT dht(PINO_DHT, DHT11);

void setup() {
  pinMode(PINO_LED, OUTPUT);
  pinMode(PINO_LDR, INPUT);
  pinMode(PINO_DHT, INPUT);
  analogWrite(PINO_LED,ledValue);
  Serial.begin(115200);
  dht.begin();
  //processCommand("GET_LDR");
}

void loop() {
  // Se receber algo pela serial
  if (Serial.available() > 0) {
    // Lê toda string recebida
    String teclado = leStringSerial();
    // chama a funcao de processar string para comando no circuito
    processCommand(teclado);
  }
}


void processCommand(String command) {
  // Verifica se o comando passado contém SET_LED e apos o espaco seja um valor numericoó
  String valorAposComando = command.substring(8, command.length());
  if (command.indexOf("SET_LED ") >= 0 && isNumero(valorAposComando)) {
    //caso seja verdadeiro, transforma o valor da string após o espaco
    ledValue = valorAposComando.toInt();
    //verifica se esse valor esta no range de 0 a 100
    if (ledValue >= 0 && ledValue <= 100 ) {
      // valor maior que 0 ,  liga led
      if (ledValue > 0 ) {
        ledUpdate(); 
        Serial.println("RES SET_LED 1");
      } else {
        // valor igual a 0 desliga o led
        ledUpdate();
        Serial.println("RES SET_LED 1");
      }

    } else
      Serial.println("RES SET_LED-1");
    // caso o range passado seja invalido

  } else if (command.indexOf("GET_LED") >= 0)
    //  retorna o valor do led atual
      Serial.printf("RES GET_LED %d\n", ledGetValue());
    else if (command.indexOf("GET_LDR") >= 0)
      Serial.printf("RES GET_LDR %d\n", ldrGetValue());
   else if (command.indexOf("GET_TEMP") >= 0){
      float temperatura = dht.readTemperature();
      Serial.printf("RES GET_TEMP %2.f\n", temperatura);
    }else if (command.indexOf("GET_HUM") >= 0){
      float umidade = dht.readHumidity();
      Serial.printf("RES GET_HUM %2.f\n", umidade);
    }
  else
    Serial.println("ERR Unknown command.");

}


// Função para atualizar o valor do LED
void ledUpdate() {
  // Normalize o valor do LED antes de enviar para a porta correspondente
  int val = map(ledValue, 0, 1023, 0, 255);
//  val = map(ledValue, 0, 1023, 0, 255);
  analogWrite(PINO_LED, val);
 
}


// Função para ler o valor do LDR
int ldrGetValue() {
  int sensorValue = analogRead(PINO_LDR);
  return sensorValue;
  // Leia o sensor LDR e retorne o valor normalizado
  
}



// Função para ler o valor do LED
int ledGetValue() {
  return ledValue;
}



/**
   Função que lê uma string da Serial
*/

String leStringSerial() {
  String conteudo = "";
  char caractere;
  // Enquanto receber algo pela serial
  while (Serial.available() > 0) {
    // Lê byte da serial
    caractere = Serial.read();
    // Ignora caractere de quebra de linha
    if (caractere != '\n') {
      // Concatena valores
      conteudo.concat(caractere);
    }
    // Aguarda buffer serial ler próximo caractere
    delay(20);
  }
  return conteudo;
}



bool isNumero(String str) {

  for (int i = 0; i < str.length(); i++) {
    if (!isdigit(str.charAt(i)))
      return false;
  }
  return true;

}
