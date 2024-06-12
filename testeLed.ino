 
int ledPin = 23;
float sinVal;
int ledVal=10;
 
void setup() 
{
  pinMode(ledPin, OUTPUT);
}
 
void loop() 
{
  //normalize = map(ledVal, 0, 100, 0, 255);
  for (int x=0; x<180; x++) 
  {
    // converte graus para radianos e então obtém o valor do seno
    sinVal = (sin(x*(3.1412/180)));
    ledVal = int(sinVal*255);
    analogWrite(ledPin, ledVal);
    delay(25);
  }
}
