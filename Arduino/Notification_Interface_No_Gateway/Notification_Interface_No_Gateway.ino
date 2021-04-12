
#include <CAN.h>
#include <OBD2.h>
#include <jled.h>
#include <SPI.h>
#include <RH_RF95.h>

// for Feather32u4 RFM9x
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 7

// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 915.0

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);



const int led_pin_r = 11;
const int led_pin_g = 9; //broken
const int led_pin_b = 10;

auto led = JLed(9);
unsigned long lastTime;



const int MAX_OBSERVERS = 10;

//int i = 0;
//bool flip = false;
//float read_pid(uint8_t PID){
//  if(!flip) i++;
//  else i --;
//  if(i==15) flip = true;
//  return i;
//}

float read_pid(uint8_t pid){
  float pidValue = OBD2.pidRead(pid);
  if (isnan(pidValue)) {
    Serial.print("error");
  } else {
    // return valid pidValue
    return pidValue;
  }
}

class Notify {
  bool add_queue(uint16_t ID, float value_1, float value_2);
};

//Holds observer settings and observations
struct Observer {
  uint16_t ID;
  bool active = false;
  int time_since_met = 0;
  int req_active_time;
  uint8_t PID;
  float req_value;
  int timeout;
  float peak_value;
  float avg_value;
  int count_since_met;
  bool compare_option; //Specify when req value is met. TRUE for actual>req_value, FALSE for actual<req_value
};

//Class for all things related to observing. Create and manage observers. 
class Observe_Class {
  public:
  Observer observers[MAX_OBSERVERS]; //Array of observers
  int observer_top = 0;  //maintain stack top to quickly add observers. 

  void add(uint16_t ID, uint8_t PID, float req_value, int req_active_time, int timeout, bool compare_option){
    observers[observer_top].ID = ID;
    observers[observer_top].PID = PID;
    observers[observer_top].req_value = req_value;
    observers[observer_top].req_active_time = req_active_time;
    observers[observer_top].timeout = timeout;
    observers[observer_top].compare_option = compare_option;
    observer_top++;
  }
  void edit(uint16_t ID, uint8_t PID, float req_value, int req_active_time, int timeout, bool compare_option){
    remove(ID);
    add(ID,PID, req_value, req_active_time, timeout, compare_option);
  }

  //remove observer. (observer removed in middle requires next observers to be shifted. 
  bool remove(uint16_t ID){
    bool found = false;
    for(int i = 0; i < observer_top; i ++){
      if(!found){
        if(observers[i].ID == ID) found = true;
      }
      else{
        observers[i-1] = observers[i];
      }
    }
    if(found) observer_top --;
    return found;
  }
  void update(){
    for(int i = 0; i < observer_top; i ++){
      float current_value = read_pid(observers[i].PID);
      long timePassed = millis() - lastTime;
      lastTime = millis();
      //if value meets required value. 
      if((current_value >= observers[i].req_value && observers[i].compare_option) 
      || (current_value <= observers[i].req_value && !observers[i].compare_option)){
        //set record value depending on compare option (greater/less than)
        if(observers[i].compare_option){
          if(current_value > observers[i].peak_value)
          observers[i].peak_value = current_value;
        }
        else{
          if(current_value < observers[i].peak_value)
          observers[i].peak_value = current_value;
        }

        //set average value with weight according to count_since_met
        observers[i].avg_value = 
       (observers[i].avg_value * observers[i].count_since_met + current_value)/(++observers[i].count_since_met);

        //make active if req value has been met for float enough
        if(!observers[i].active){
          if(observers[i].time_since_met >= observers[i].req_active_time){
            observers[i].active = true;
          }
        }
        else{
          //check if timeout should take place. Send notification if so. 
          if(observers[i].time_since_met >= observers[i].timeout){
            notify(observers[i].ID,observers[i].peak_value, observers[i].avg_value);
            observers[i].active = false;
            observers[i].time_since_met = 0;
            observers[i].count_since_met = 0;
            observers[i].peak_value = 0;
          }
        }
        
        //increase time since met
        observers[i].time_since_met += (timePassed/1000); //add real-time to time_since_met
      }
      else{//values out of range. 
        
        //if notification was being recorded. 
        if(observers[i].active){
        observers[i].active = false;
        notify(observers[i].ID,observers[i].peak_value, observers[i].avg_value);
        }
        //reset time since met
        observers[i].time_since_met = 0;
        observers[i].count_since_met = 0;
        observers[i].peak_value = 0;
        
      }
    }
  }

  
};

Observe_Class observe;

void notify(uint16_t ID, float val1, float val2){
  String str_msg;
  str_msg += (String)ID;
  str_msg += " ";
  str_msg += (String) val1;
  str_msg += " ";
  str_msg += (String) val2;
  char message[30];
  str_msg.toCharArray(message,30);
  Local_TX(message);
  Serial.println(message);
  {
    led = JLed(led_pin_r).Blink(300,300).Repeat(2);
  }
}

void Local_TX(char message[]){
    Serial.print("Sending "); Serial.println(message);
    delay(10);
    rf95.send((uint8_t *)message, 20);

    delay(10);
    rf95.waitPacketSent();
    Serial.print("Sent!");
}


//process string based commands. 
bool run_command(char command[30]){
  //tokenize command for argument passing
  byte index = 0;
  char *p[30];
  char *ptr = NULL;
  ptr = strtok(command, ":;,\n");
  
  while(ptr!=NULL){
    p[index] = ptr;
    index++;
    ptr =  strtok(NULL, ":;,\n");
  }

  switch (*p[0]) {
  case 'A': //add observver
    Serial.println("Add");
    observe.add(atoi(p[1]),atoi(p[2]),atof(p[3]),atoi(p[4]),atoi(p[5]),p[6]!=0);
    break;
  case 'E': //edit observer
    Serial.println("Edit");
    observe.edit(atoi(p[1]),atoi(p[2]),atof(p[3]),atoi(p[4]),atoi(p[5]),p[6]!=0);
    break;
  case 'D': //delete observer
    Serial.println("Delete");
    observe.remove(atoi(p[1]));
    break;
  case 'P': //print PID
    int PID = atoi(p[1]);
    float PID_val = read_pid(PID);
    notify(0,PID, PID_val); //send pid notification
    break;
  default:
    Serial.println("Invalid command");
    break;
}

}

//get commands, if any
void get_commands(){

  //get array of characters from serial if available
  char command[30];
  int i = 0;
  if(Serial.available() > 0){
    while(Serial.available()){
      delay(2);
      command[i] = Serial.read();
      i++;
    }
    command[i] = '\0';
    run_command(command);
  }

  //get array of characters from LoRa if available
   if (rf95.available())
  {
    Serial.print("Got Lora Command: ");
    // Should be a message for us now
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    if (rf95.recv(buf, &len))
    {
      Serial.println((char*)buf);
    }
    else
    {
      Serial.println("Receive failed");
    }
    run_command((char*)buf);
  }
  
}

void setup() {
  // initialize serial at 9600 baud
  Serial.begin(9600);
  //while (!Serial);

  //connect to LoRa
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  
  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    Serial.println("Uncomment '#define SERIAL_DEBUG' in RH_RF95.cpp for detailed debug info");
    while (1);
  }
  Serial.println("LoRa radio init OK!");


  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);

  rf95.setTxPower(23, false);
  


  //connect to can
  while (true) {
    Serial.print(F("Attempting to connect to OBD2 CAN bus ... "));
    CAN.setPins(6,2);
    if (!OBD2.begin()) {
      Serial.println(F("failed!"));
      delay(1000);
    } 
    else {
      led = JLed(led_pin_b).On();
      led.Update();
      Serial.println(F("success"));
      break;
    }
  }
  Serial.println();
}

//main loop
void loop() {
  get_commands();
  led.Update();
  observe.update();
}
