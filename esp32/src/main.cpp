#include <Arduino.h>

// Constants
const int baudrate = 115200;
const int ledPin = 4;
const int ldrPin = 34;
const float min_percentage = 0.0;
const float max_percentage = 100.0;
const float resolution = 4095.0;
const int channel = 0;
const int freq = 5000;     
const int resolution_pwm = 8;

// Global variables
int threshold = 0;
int currentLedPct = 0;

// Function headers
void print_ldr(void);
void automatic_led(void);
void get_threshold(void);
void error_command(void);
void set_threshold(String input);
void get_ldr(void);
void get_led(void);
void set_led(String input);
void setLedPercent(int pct);

// Setup Arduino Framework
void setup() {
  Serial.begin(baudrate);
  Serial.println("Ping! I'm alive!");
  ledcSetup(channel, freq, resolution_pwm);
  ledcAttachPin(ledPin, channel);
  pinMode(ldrPin, INPUT);
}

// Loop Arduino Framework
void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.replace("\r", "");
    input.trim();
    if (input.length() == 0) return; // ignore trash
    if (input.startsWith("SET_LED ")) set_led(input);   
    else if (input.startsWith("GET_LED")) get_led();
    else if (input.startsWith("GET_LDR")) get_ldr();
    else if (input.startsWith("SET_THRESHOLD ")) set_threshold(input);
    else if (input.startsWith("GET_THRESHOLD")) get_threshold();
    else error_command();
  }
  
  automatic_led();
  print_ldr();
  delay(2000);
}

void print_ldr(void) {
  int ldr_pwm = analogRead(ldrPin);
  float ldr_pct = (ldr_pwm * max_percentage) / resolution;
  Serial.println("GET_LDR " + String(int(ldr_pct)));
}

void automatic_led(void) {

  int ldr = analogRead(ldrPin);
  float ldr_pct = (ldr * max_percentage) / resolution;
  if (ldr_pct > threshold) {
    setLedPercent(currentLedPct);
  } else {
    setLedPercent(0);
  }
}

void get_threshold(void) {
  /* 
  Retorna o valor atual do threshold.
  */
  
  Serial.println("RES GET_THRESHOLD " + String(threshold));
}

void error_command(void) {
  Serial.println("ERR Unknown command.");
}

void set_threshold(String input) {
  /* 
  Define o limiar de ativação automática do LED (X entre 0-100). 
  O LED acende automaticamente quando a leitura normalizada do LDR 
  ultrapassar esse valor. 
  */

  String threshold_str = input.substring(14);
  threshold = threshold_str.toInt();
  Serial.println("RES SET_THRESHOLD 1");
}

void get_ldr(void) {
  /* 
  Retorna o valor da leitura atual do LDR 
  (normalizado entre 0-100)
  */

  int ldrValue = analogRead(ldrPin);
  float percentage = (ldrValue * max_percentage) / resolution;
  Serial.println("RES GET_LDR " + String(int(percentage)));
}

void get_led(void) {
  /*
  Retorna a intensidade atual do LED.
  */

  Serial.println("RES GET_LED " + String(currentLedPct));
}

void set_led(String input) {
  /*
  Define a intensidade do LED (X entre 0-100). Valores fora 
  desse intervalo devem ser ignorados e retornar erro.
  */

  String valueStr = input.substring(8);
  int luminosity = valueStr.toInt();

  
  if (luminosity >= min_percentage && luminosity <= max_percentage) {

    currentLedPct = luminosity;
    setLedPercent(luminosity);

    Serial.println("RES SET_LED 1");
  }
  else {
    Serial.println("RES SET_LED -1");
  }
}

void setLedPercent(int pct) {
  int duty = (pct * 255) / 100;
  ledcWrite(channel, duty);
}