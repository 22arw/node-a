
#include <CAN.h>
#include <OBD2.h>
#include <jled.h>
#include <SPI.h>
#include <TinyLoRa.h>

// Network Session Key (MSB)
uint8_t NwkSkey[16] = { 0x19, 0x78, 0xb6, 0x13, 0x87, 0x6b, 0x6b, 0x1a, 0xee, 0xb1, 0x35, 0x65, 0xdd, 0xea, 0x39, 0x73 };


// Application Session Key (MSB)
uint8_t AppSkey[16] = { 0x5f, 0xb2, 0xe3, 0x39, 0xf2, 0x7b, 0x28, 0xcd, 0x93, 0x36, 0x91, 0xf2, 0x00, 0xde, 0xa5, 0x5a };

// Device Address (MSB)
uint8_t DevAddr[4] = { 0x00, 0x41, 0x23, 0x9a };


// for Feather32u4 RFM9x
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 7

TinyLoRa lora = TinyLoRa(RFM95_INT, RFM95_CS, RFM95_RST);


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

typedef union {
  unsigned long longFormat;
  float floatFormat;
}ByteFloatUnion;

void notify(uint16_t ID, float val1, float val2){
  unsigned char loraData[10];

  //Store float as long (not the same as casting, as this would round)
  ByteFloatUnion byteVal1;
  ByteFloatUnion byteVal2;
  
  byteVal1.floatFormat = val1;
  byteVal2.floatFormat = val2;
  
  loraData[0] = lowByte(ID);
  loraData[1] = highByte(ID);
  loraData[2] = (byteVal1.longFormat) & 0xff;
  loraData[3] = (byteVal1.longFormat>>8) & 0xff;
  loraData[4] = (byteVal1.longFormat>>16) & 0xff;
  loraData[5] = (byteVal1.longFormat>>24) & 0xff;
  loraData[6] = (byteVal1.longFormat) & 0xff;
  loraData[7] = (byteVal1.longFormat>>8) & 0xff;
  loraData[8] = (byteVal1.longFormat>>16) & 0xff;
  loraData[9] = (byteVal1.longFormat>>24) & 0xff;
 
  lora.sendData(loraData, sizeof(loraData), lora.frameCounter);
  lora.frameCounter++;
  String str_msg;
  str_msg += (String)ID;
  str_msg += " ";
  str_msg += (String) val1;
  str_msg += " ";
  str_msg += (String) val2;
  char message[30];
  str_msg.toCharArray(message,30);
  Serial.println(message);
  {
    led = JLed(led_pin_r).Blink(300,300).Repeat(2);
  }
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

  //Commands over LoraWAN not supported.
  
}

void setup() {
  // initialize serial at 9600 baud
  Serial.begin(9600);
  //while (!Serial);

    // Initialize LoRa
  Serial.print("Starting LoRa...");
  // define channel to send data on
  lora.setChannel(CH2);
  // set datarate
  lora.setDatarate(SF7BW125);
    if(!lora.begin())
  {
    Serial.println("Failed");
    Serial.println("Check your radio");
    while(true);
  }
  Serial.println("LoRa Ready!");
 
  
  //connect to can
  while (true) {
    Serial.print(F("Attempting to connect to OBD2 CAN bus ... "));
    CAN.setPins(6,2);
    if (!OBD2.begin()) {
      Serial.println(F("failed!"));
      delay(1000);
    } 
    else {2
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
  //get_commands();
  //led.Update();
  //observe.update();
  float val1 = 0.123;
  float val2 = 0.456;
  int ID = 10;
  notify(ID, val1, val2);
  delay(20000);
}
