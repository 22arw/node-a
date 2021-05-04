#include <CAN.h>
#include <OBD2.h>
#include <jled.h>
#include <SPI.h>
#include <TinyLoRa.h>
#include <Wire.h>
#include "Adafruit_FRAM_I2C.h"

// for Feather32u4 RFM9x
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 7

TinyLoRa lora = TinyLoRa(RFM95_INT, RFM95_CS, RFM95_RST);
Adafruit_FRAM_I2C fram     = Adafruit_FRAM_I2C();

int pushButton = A0;

const int led_pin_r = 9;
const int led_pin_g = 11; 
const int led_pin_b = 10;

auto led = JLed(9);

void test_fram() {
  Serial.println("Attempting to connect to FRAM chip.");
  if (fram.begin()) {
    Serial.println("Success! Fram chip found.");
  }
  else{
    Serial.println("Failed - fram chip not identified. Check connections.");
  }
}
void test_can_shield(){
    //connect to can
    Serial.println("Attempting to connect to OBD2 CAN bus ... ");
    CAN.setPins(6,2);
    if (!CAN.begin(500E3)) {
    Serial.println("Failed - check CAN shield connection");
    }
    else {
    Serial.println("Success! - CAN shield connected.");
    }
}

void test_lora(){
  // define channel to send data on
  lora.setChannel(CH2);
  // set datarate
  lora.setDatarate(SF7BW125);

  Serial.println("Testing loRa radio connection...");
    if(!lora.begin())
  {
    Serial.println("Failed - LoRa failed to initialize. ");
    Serial.println("Check all connections to loRa radio.");

  }
    else 
    {
    Serial.println("Success! LoRa connected.");
    }
   
}


void test_led(){
  
  Serial.println("Testing RGB LED connection...");
  Serial.println("Press any key to test red.");
  while(true){
    if(Serial.available()) {
      while(Serial.available()){
        Serial.read();
      }
      break;
    }
  }

  //Test red
  led = JLed(led_pin_r).Blink(300,300).Repeat(1000);
  Serial.println("Is the LED blinking red? (y/n)");
  while(true){
    if(Serial.available()) {
      while(Serial.available()){
        if(Serial.read()=="y"){
          Serial.println("Success!");
        }
        else if(Serial.read()=="n"){
          Serial.println("Failed - Please check LED wiring.");
        }
        else {
          Serial.read();
        }
        
      }
      break;
    }
    delay(20);
    led.Update();
  }

  //Test blue
  led = JLed(led_pin_b).Blink(300,300).Repeat(1000);
  Serial.println("Is the LED blinking blue? (y/n)");
  while(true){
    if(Serial.available()) {
      while(Serial.available()){
        if(Serial.read()=="y"){
          Serial.println("Success!");
        }
        else if(Serial.read()=="n"){
          Serial.println("Failed - Please check LED wiring.");
        }
        else {
          Serial.read();
        }
        
      }
      break;
    }
    delay(20);
    led.Update();
  }

 //Test green
  led = JLed(led_pin_g).Blink(300,300).Repeat(1000);
  Serial.println("Is the LED blinking green? (y/n)");
  while(true){
    if(Serial.available()) {
      while(Serial.available()){
        if(Serial.read()=="y"){
          Serial.println("Success!");
        }
        else if(Serial.read()=="n"){
          Serial.println("Failed - Please check LED wiring.");
        }
        else {
          Serial.read();
        }
        
      }
      break;
    }
    delay(20);
    led.Update();
  }
  
  
  
  
}
int buttonState;
int lastButtonState;
void setup() {
  //Start serial with 9600 baud
  Serial.begin(9600);
  
  pinMode(pushButton, INPUT_PULLUP);
  buttonState = digitalRead(pushButton);
  lastButtonState = buttonState;
}

void loop() {
  buttonState = digitalRead(pushButton);
  if(buttonState!=lastButtonState){
    if(buttonState==1){
      Serial.println("Button released");
    }
    if(buttonState==0){
      Serial.println("Button pushed");
    }
    lastButtonState = buttonState;
  }
  
  while(Serial.available()>0){
    switch(Serial.read()) {
      case 'l':
      test_led();
      break;
      case 'c':
      test_can_shield();
      break;
      case 'r':
      test_lora();
      break;
      case 'f':
      test_fram();
      break;
      default:
      break;
    }
  }

}
