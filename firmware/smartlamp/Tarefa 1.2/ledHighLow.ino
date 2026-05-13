#define LED_PIN 2;

int i = 0;

void setup(){
    pinMode(LED_PIN, OUTPUT);
}

void loop(){
    if(i < 5){
        i++;
        digialWrite(LED_PIN, LOW);
        delay(2000);
        digialWrite(LED_PIN, HIGH);
        delay(2000);

    }
    digialWrite(LED_PIN, LOW);

}