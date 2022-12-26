#include <LiquidCrystal_I2C.h>
#include <NewPing.h>
#include <Wire.h> 
#include <Keypad.h>
#include <MFRC522.h>
#include <SPI.h>
#include <IRremote.h>

#define IR_RECEIVE_PIN 0
#define RST_PIN         9          
#define SS_PIN          10         

//initialize components
//RFID
MFRC522 mfrc522(SS_PIN, RST_PIN); 

//LCD display
LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display

//ustrasonic sensor
#define TRIGGER_PIN  16  
#define ECHO_PIN     15  
#define MAX_DISTANCE 200 
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); 

//leds
const int ledPin = 14;
const int greenled = 17;
int ledState = LOW;           

//keypad
const byte ROWS = 4; 
const byte COLS = 4; 
char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {8, 7, 6, 5}; 
byte colPins[COLS] = {4, 3, 2, 1}; 
Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 

//declarations
unsigned long previous=0; // will store last time LED was updated
unsigned long now=0;   
unsigned long seeyou=0;
const long interval_1 = 10000;    
const long interval_2 = 100;  
char code[4]; //used to check password
int count;
int count2 = 0;
bool flag = true;

//function to be called when alarm is on 
void alarmon(){
    lcd.init(); //lcd set up
    lcd.backlight();
    lcd.setCursor(0,0);
    lcd.print("ALARM ON!!!");
    lcd.setCursor(0,1);
    lcd.print("Enter password.");
    previous = millis();
    now = millis();
    count = 0;
    while(1){
       if (now - previous >= interval_2) {  //led blinking and buzzer on
          previous = now;
          if (ledState == LOW) {
             ledState = HIGH;          
             tone(ledPin, 659); 

          } 
          else {
             ledState = LOW;
             noTone(ledPin);
          }          
      }
      now = millis();
      char key = customKeypad.getKey(); //check if any password was given
      if (key){
        if (key!='#'){
          code[count] = key;
          count++;
          if (count == 4){ //4-digit password
            if (code[0] == '1' or code[1] == '2' or code[2] == '3' or code[3] == '4'){ //if it was the correct password (e.g. here '1234')
              welcome(); //call welcome() - unlock the system
              break;
            }
            else {
              count=0;
            }
          }
        }
        else{
          count = 0;
        }
      }
      if ( ! mfrc522.PICC_IsNewCardPresent()) { //check if there is any RFID card
                  continue;
      }
      if ( ! mfrc522.PICC_ReadCardSerial()) { //and if it unlocks the system (by checking card's UID)
                  continue;
      }
      String content= "";
      byte letter;
      for (byte i = 0; i < mfrc522.uid.size; i++) 
      {
        content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
        content.concat(String(mfrc522.uid.uidByte[i], HEX));
      }
      content.toUpperCase();
      if (content.substring(1) == "8A F0 9D 7F" || content.substring(1) == "63 A9 FC 02") 
      {
         welcome();
         break;
      }
    }
}

//function to be called when right password/card was given. It resets the components and after that the system is off.
void welcome(){
    noTone(ledPin);    
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0,0);
    lcd.print("Welcome!");
    digitalWrite(greenled, HIGH);
    delay(5000);
    digitalWrite(greenled, LOW);
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0,0);
    lcd.print("System Off.");
    IrReceiver.resume();
}

void setup() {
  //Serial.begin(115200); // Open serial monitor at 115200 baud to see ping results.
  pinMode(ledPin, OUTPUT);
  tone(ledPin, 1000, 2000);
  pinMode(greenled, OUTPUT);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("System Off.");
  SPI.begin();
  mfrc522.PCD_Init();
  IrReceiver.begin(IR_RECEIVE_PIN);
  int i=0;
}

void loop() {
  flag = true;
  char key = customKeypad.getKey(); //get password
  if (key){
    if(key!='#'){
      code[count2] = key;
      count2++;
      if (count2 == 4){ //check 4-digit password that was given
          if (code[0] == '1' and code[1] == '2' and code[2] == '3' and code[3] == '4'){ //correct password ('1234')
            count2 = 0;
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print("ARMED"); //arm the system by giving the password while it was off
            IrReceiver.end();
            IrReceiver.begin(IR_RECEIVE_PIN);
            bool flag2 = false;
            while (sonar.ping_cm()>=10){  //while sensor does not detect anyone
              flag2 = false;
              previous = millis();
              while(millis()-previous < 500){ //check if receiver receives the right signal -> the door is closed (repeat for synchronization)
                if(IrReceiver.decode()){
                  if(IrReceiver.decodedIRData.decodedRawData == 0xCB340102){ //signal set for communication
                    flag2 = true;
                    IrReceiver.resume();
                    break;
                  }
                }
                IrReceiver.resume();
              }
              if (flag2 == true) continue;
              else{ //it didn't receive the right signal from the other side
                break;
              }
            }
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print("SECURITY BREACH!");
            digitalWrite(ledPin, LOW);
            lcd.setCursor(0,1);
            lcd.print("Enter password.");
            seeyou = millis();
            previous = seeyou;
            now = millis();
            count = 0;
            while (now - seeyou < interval_1){ //gives 10 secs to put the right password/card
               if (now - previous >= 1000) { //led blinks and buzzer 
                  previous = now;
                  if (ledState == LOW) {
                     ledState = HIGH;
                     tone(ledPin, 440); 
                  } 
                  else {
                     ledState = LOW;
                     noTone(ledPin);
                  }
              }
              now = millis();
              char key = customKeypad.getKey(); //check for password
              if (key){
                if (key!='#'){
                  code[count] = key;
                  count++;
                  if (count == 4){
                    if (code[0] != '1' or code[1] != '2' or code[2] != '3' or code[3] != '4'){
                      alarmon();
                      flag=false;
                      break;
                    }
                    else {
                      welcome();
                      flag=false;
                      break;
                    }
                  }
                }
                else {
                  count = 0;
                }
              }
              if ( ! mfrc522.PICC_IsNewCardPresent()) { //check for cards
                  continue;
              }
              if ( ! mfrc522.PICC_ReadCardSerial()) {
                  continue;
              }
              String content= "";
              byte letter;
              for (byte i = 0; i < mfrc522.uid.size; i++) 
              {
                 content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
                 content.concat(String(mfrc522.uid.uidByte[i], HEX));
              }
              content.toUpperCase();
              if (content.substring(1) == "8A F0 9D 7F" || content.substring(1) == "63 A9 FC 02") 
              {
                welcome();
                flag = false;
                break;
              }
             
             else   {
                alarmon();
                flag=false;
                break;
              }
          }
          if (flag==true) 
            alarmon();
      }
      else {
        count2 = 0;
      }
    }
  }
  else{ //reset pushed keys
    count2 = 0;
  }
  }
  }
