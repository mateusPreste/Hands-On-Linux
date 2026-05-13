#define LDR_PIN 15

void setup() {
  Serial.begin(115200);
  pinMode(LDR_PIN, INPUT);
  Serial.println("Sistema iniciado - Lendo luminosidade...");
}

void loop() {
  int valorLDR = analogRead(LDR_PIN);

  Serial.print("Valor Lido: ");
  Serial.println(valorLDR);

  delay(500);
}
