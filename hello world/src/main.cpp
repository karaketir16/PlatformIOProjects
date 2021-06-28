#include <Arduino.h>
void ser() 
{
  Serial.setRx(PA10); // using pin name PY_n
  Serial.setTx(PA9); // using pin number PYn
}

void setup() {
  ser();
  Serial.begin(9600);
  // put your setup code here, to run once:
}

void loop() {
  Serial.println("Hello World");
  // put your main code here, to run repeatedly:
}