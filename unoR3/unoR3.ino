#include <dht.h>

dht DHT;

#define DHT11_PIN 5
#define HEATER 13
#define WINDOW_SENSOR_PIN 8
#define MAINLIGHT 11
#define MIRRORLIGHT 10
#define LAMPLEFT 2
#define LAMPRIGHT 3
#define DOOR_SENSOR_PIN 9

// desired light states
boolean main_light_should_be_on = false;
boolean mirror_light_should_be_on = false;
boolean left_lamp_should_be_on = false;
boolean right_lamp_should_be_on = false;
// override desired light states
boolean override_main_light = false;
boolean override_mirror_light_should_be_on = false;
boolean override_left_lamp_should_be_on = false;
boolean override_right_lamp_should_be_on = false;

// actual light states
boolean main_light_on = false;
boolean lamp_left_on = false;
boolean lamp_right_on = false;
boolean mirror_light_on = false;

boolean door_light_active = true; // controls if the mirror light should come on when the door opens

int desired_temperature = 18;

boolean heater_on = false;
boolean window_open = false;
boolean door_open = false;

unsigned long heater_on_ms = 0;
unsigned long last_temp_check = 0;
unsigned long last_door_check = 0;
unsigned long door_light_timer = 0;
unsigned long mirror_light_on_ms = 0;

String inputString = "";         // a string to hold incoming data
String heater_ctrl_on = "HTRON\n";
String heater_ctrl_off = "HTROFF\n";
String main_ctrl_on = "MNLGTON\n";
String main_ctrl_off = "MNLGTOFF\n";
String lamps_ctrl_on = "LMPSON\n";
String lamps_ctrl_off = "LMPSOFF\n";
boolean stringComplete = false;  // whether the string is complete

String sleep_state_ctrl = "sleep state\n";
String standby_state_ctrl = "standby state\n";
String all_on_state_ctrl = "all on state\n";

//MIRROR values
int mirrorState = HIGH;
long previous_mirror_ms = 0;
unsigned long current_mirror_ms = millis();
long mirror_on_time_ms = 5000;

int mode = 1;

void setup(){
  pinMode (WINDOW_SENSOR_PIN, INPUT_PULLUP);
  pinMode (DOOR_SENSOR_PIN, INPUT_PULLUP);
  pinMode (LAMPLEFT, OUTPUT);
  pinMode (LAMPRIGHT, OUTPUT);
  pinMode (HEATER, OUTPUT);
  pinMode (MAINLIGHT, OUTPUT);
  pinMode (MIRRORLIGHT, OUTPUT);
  
  digitalWrite(HEATER, HIGH);
  digitalWrite(MAINLIGHT, HIGH );
  digitalWrite(LAMPLEFT, LOW);
  digitalWrite(LAMPRIGHT, LOW);
  digitalWrite(MIRRORLIGHT, HIGH);
  
  // turn heater off, so its state is known
  
  //window_open = digitalRead(WINDOW_SENSOR_PIN);
  Serial.begin(9600);
  int digitalRead = DHT.temperature;
  
}

void change_state(int state_code) {
  mode = state_code;
  override_main_light = false;
}

void loop(){

   // standby mode
   //    lights off
   //    heater minium 8 degrees

   switch (mode) {
     case 1: // standby
       // ensure all lights are off
       if (override_main_light) {
          main_light_should_be_on = true;
       } else {
          main_light_should_be_on = false;
       }
       left_lamp_should_be_on = false;
       right_lamp_should_be_on = false;
       door_light_active = true;
       desired_temperature = 8;
       break;
     case 2: // all on
       if (override_main_light) {
          main_light_should_be_on = false;
       } else {
          main_light_should_be_on = true;
       }       left_lamp_should_be_on = true;
       right_lamp_should_be_on = true;
       door_light_active = false;
       desired_temperature = 20;
       break;
     case 3: // sleep
       door_light_active = false;
       if (override_main_light) {
          main_light_should_be_on = true;
       } else {
          main_light_should_be_on = false;
       }       left_lamp_should_be_on = false;
       right_lamp_should_be_on = false;
       desired_temperature = 18;
       break;
     case 4: // tv
       door_light_active = false;
       if (override_main_light) {
          main_light_should_be_on = true;
       } else {
          main_light_should_be_on = false;
       }       left_lamp_should_be_on = true;
       right_lamp_should_be_on = true;
       desired_temperature = 20;
       break;
     default:
     break;
   }

   // set lights to desired states
   if (main_light_should_be_on != main_light_on) {
     if(main_light_should_be_on) {
       digitalWrite(MAINLIGHT, LOW);
       main_light_on = true;
     } else {
       digitalWrite(MAINLIGHT, HIGH);
       main_light_on = false;
     }
   }

   
   if (left_lamp_should_be_on != lamp_left_on) {
     if(left_lamp_should_be_on) {
       digitalWrite(LAMPLEFT, HIGH);
       lamp_left_on = true;
     } else {
       digitalWrite(LAMPLEFT, LOW);
       lamp_left_on = false;
     }
   }
   if (right_lamp_should_be_on != lamp_right_on) {
     if(right_lamp_should_be_on) {
       digitalWrite(LAMPRIGHT, HIGH);
       lamp_right_on = true;
     } else {
       digitalWrite(LAMPRIGHT, LOW);
       lamp_right_on = false;
     }
   }

   if (door_light_active) {
     door_open = digitalRead(DOOR_SENSOR_PIN);
     if (door_open) {
        // turn on the mirror light and record when we turned it on
        digitalWrite(MIRRORLIGHT, LOW);
        mirror_light_on = true;
        mirror_light_on_ms = millis();
     }

     // check mirror light is turned on and it has been on for over mirror_on_time_ms
     if (mirror_light_on and (millis() - mirror_light_on_ms > mirror_on_time_ms)) {
        // turn the light off
        digitalWrite(MIRRORLIGHT, HIGH);
        mirror_light_on = false;
     }
   }


   if (stringComplete) {
     if (inputString == sleep_state_ctrl) {
       change_state(3);
     } else if (inputString == standby_state_ctrl) {
       change_state(1);
     } else if (inputString == all_on_state_ctrl) {
       change_state(2);
     } else if (inputString == main_ctrl_on) {
       Serial.println("main light ctrl on recv");
       if (main_light_on) {
         Serial.println("main light already on");
         // nothing to do 
       } else {
         Serial.println("main light is off");
         // toggles the main light override
         override_main_light = !override_main_light;
       }
     } else if (inputString == main_ctrl_off) {
       Serial.println("main light ctrl off recv");
       if (main_light_on) {
         // toggles the main light override
         override_main_light = !override_main_light;
       } else {
         // nothing to do
       }
     }
     inputString = "";
     stringComplete = false;
   }

     // check the last time we ran the heater code
   if (millis() - last_temp_check > 3000) {
      // it's been more than 3 seconds 
      int chk = DHT.read11(DHT11_PIN);
      Serial.print("TMP");
      Serial.println(DHT.temperature);
    
      if (DHT.temperature < desired_temperature && !heater_on){
        Serial.println("HTRON");
        digitalWrite(HEATER, LOW);
        heater_on = true;
        heater_on_ms = millis();
        // else if the temperature is too high and the heater is on
      } else if (DHT.temperature > (desired_temperature +1) && heater_on) {
        Serial.println("HTROFF");
        digitalWrite(HEATER, HIGH);
        heater_on = false;
      }
      last_temp_check = millis(); 
  }   
}

void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}
