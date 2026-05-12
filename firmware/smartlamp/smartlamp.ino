// Definição de Pinos e Variáveis
#define LED_PIN 2      
#define LDR_PIN 15     

int ledValue = 0;           // Intensidade (0-100)
int thresholdValue = 50;    // Limiar de ativação (0-100)
const int ldrMax = 4095;    // Valor maximo lido

unsigned long previousMillis = 0; 
const long interval = 2000; // Intervalo de 2 segundos

//Configuração de inicialização
void setup() 
{
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(LDR_PIN, INPUT);
  
  Serial.printf("SmartLamp Initialized.\n");
  
  ledUpdate(); // Garante o estado inicial do LED
}

void loop() 
{
  // Leitura dos comandos da Serial
  if (Serial.available() > 0) 
  {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim(); 
   
    if (cmd.length() > 0) 
    {
      processCommand(cmd); 
    }
  }

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) 
  {
    previousMillis = currentMillis;
    
    int currentLDR = ldrGetValue(); // Passo 5
    
    // Envio automático do valor do LDR normalizado
    Serial.print("RES GET_LDR ");
    Serial.println(currentLDR);

    // Ativação automática caso limiar seja ultrapassado
    if (currentLDR > thresholdValue) 
    {
      ledValue = 100;
    } 
    else 
    {
      ledValue = 0;
    }
    
    ledUpdate();
    
  }
}


void processCommand(String cmd) 
{
  if (cmd.startsWith("SET_LED ")) 
  {
    int val = cmd.substring(8).toInt();
    if (val >= 0 && val <= 100) 
    {
      ledValue = val;
      ledUpdate();
      Serial.println("RES SET_LED 1");
    } 
    else 
    {
      Serial.println("RES SET_LED -1");
    }
  } 
  else if (cmd == "GET_LED") 
  {
    Serial.print("RES GET_LED ");
    Serial.println(ledValue);
  } 
  else if (cmd == "GET_LDR") 
  {
    Serial.print("RES GET_LDR ");
    Serial.println(ldrGetValue());
  } 
  else if (cmd.startsWith("SET_THRESHOLD ")) 
  {
    int val = cmd.substring(14).toInt();
    if (val >= 0 && val <= 100) 
    {
      thresholdValue = val;
      Serial.println("RES SET_THRESHOLD 1");
    }
    else 
    {
      Serial.println("RES SET_THRESHOLD -1");
    }
  } 
  else if (cmd == "GET_THRESHOLD") 
  {
    Serial.print("RES GET_THRESHOLD ");
    Serial.println(thresholdValue);
  } 
  else 
  {
    Serial.println("ERR Unknown command.");
  }
}

// Função para atualizar o valor do LED
void ledUpdate() 
{
  // Converte o PWM dos valores de 0-100 para 0-255
  int pwmValue = map(ledValue, 0, 100, 0, 255);
  analogWrite(LED_PIN, pwmValue);
}

// Função para ler o valor do LDR
int ldrGetValue() 
{
  int raw = analogRead(LDR_PIN);
  // Normaliza a leitura do LDR de 0-4095 para 0-100
  return map(raw, 0, ldrMax, 0, 100);
}
