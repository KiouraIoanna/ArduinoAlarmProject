#include <IRremote.h>

#define IR_SEND_PIN 3
unsigned long previous;
long int sig = 0xCB340102; //signal to be sent to receiver

void setup()
{
   Serial.begin(9600);
   IrSender.begin(IR_SEND_PIN);
}

void loop()
{
      IrSender.sendNECRaw(sig, 0); //send the signal 
      delay(50);
}
