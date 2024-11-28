const int ldrPin = 12;
int ldrValue = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
}

void loop() {
  ldrValue = analogRead(ldrPin);
  Serial.print("LDR value: ");
  Serial.println(ldrValue);
  delay(500);
}