#include <HomeAutomationLight.h>
#include <HomeAutomationTimer.h>
#include <dht.h>

dht DHT;

const int dht_pin = 5;
const int heater_pin = 7;
const int window_sensor_pin = 8;
const int main_light_pin = 11;
const int mirror_lamp_pin = 10;
const int left_lamp_pin = 2;
const int right_lamp_pin = 3;
const int door_sensor_pin = 9;

// overrides
const int OVERRIDE_ON = 0;
const int OVERRIDE_OFF = 1;
const int DONT_OVERRIDE = 2;
// lights
HomeAutomationLight main_light(main_light_pin, false); // active low
HomeAutomationLight mirror_light(mirror_lamp_pin, false);  // active low
HomeAutomationLight left_lamp(left_lamp_pin);
HomeAutomationLight right_lamp(right_lamp_pin);

boolean door_light_active = true; // controls if the mirror light should come on when the door opens
unsigned long mirror_on_time_ms = 5000;
// create a timer to manage the turning off of the mirror light after 5 seconds
auto mirror_timer = HomeAutomationTimer(mirror_on_time_ms, 
                                        [&mirror_light](){ mirror_light.on(); },   // call mirror_light.on when triggered
                                        [&mirror_light](){ mirror_light.off(); });  // call mirror_light.off when time expires 

// desired light states
boolean mirror_light_should_be_on = false;
boolean main_light_should_be_on = false;
boolean left_lamp_should_be_on = false;
boolean right_lamp_should_be_on = false;

// override desired light state
int override_main_light = DONT_OVERRIDE;
int override_mirror_light = DONT_OVERRIDE;
int override_left_lamp = DONT_OVERRIDE;
int override_right_lamp = DONT_OVERRIDE;


int desired_temperature = 18;

boolean heater_on = false;
boolean window_open = false;
boolean door_open = false;

unsigned long heater_on_ms = 0;
unsigned long last_temp_check = 0;
unsigned long last_door_check = 0;
unsigned long mirror_light_on_ms = 0;

String inputString = "";         // a string to hold incoming data
const String heater_ctrl_on = "HTRON\n";
const String heater_ctrl_off = "HTROFF\n";
const String main_ctrl_on = "MNLGTON\n";
const String main_ctrl_off = "MNLGTOFF\n";
const String right_lamp_ctrl_on = "RLMPON\n";
const String right_lamp_ctrl_off = "RLMPOFF\n";
const String left_lamp_ctrl_on = "LLMPON\n";
const String left_lamp_ctrl_off = "LLMPOFF\n";
boolean stringComplete = false;  // whether the string is complete

const String sleep_state_ctrl = "sleep state\n";
const String standby_state_ctrl = "standby state\n";
const String all_on_state_ctrl = "all on state\n";
const String tv_state_ctrl = "tv state\n";

// heater boost variables
const String heater_boost_ctrl = "heater boost\n";
unsigned long heater_boost_ms = 5L * 60L * 1000L;  // time boost should last for
unsigned long heater_boost_enabled_ms = 0; // time boost was enabled
boolean heater_boost = false;

//MIRROR values
int mirrorState = LOW;
long previous_mirror_ms = 0;
unsigned long current_mirror_ms = millis();
boolean mirror_light_on = false;

int mode = 1;

void setup(){
  pinMode (window_sensor_pin, INPUT_PULLUP);
  pinMode (door_sensor_pin, INPUT_PULLUP);  
  pinMode (heater_pin, OUTPUT);
  // turn heater off, so its state is known
  digitalWrite(heater_pin, HIGH);
  
  //window_open = digitalRead(WINDOW_SENSOR_PIN);
  Serial.begin(9600);
  while (!Serial) ;
  int digitalRead = DHT.temperature;

  Serial.println("Starting");
  
}

void change_state(int state_code) {
  mode = state_code;
  override_main_light = DONT_OVERRIDE;
  override_left_lamp = DONT_OVERRIDE;
  override_right_lamp = DONT_OVERRIDE;
}

boolean should_light_be_on(boolean state_desired_on, int override_value) {
  boolean lamp_on = false;
  if (override_value == OVERRIDE_OFF) {
    lamp_on = false;
  } else if (override_value == OVERRIDE_ON) {
    lamp_on = true;
  } else {
    lamp_on = state_desired_on;
  }
  return lamp_on;
}

void check_door_light(boolean enabled) {
  if (enabled) {
    door_open = digitalRead(door_sensor_pin);
    if (door_open) {
      mirror_timer.trigger();
    }
  }
  mirror_timer.update();
}

void loop(){

   // standby mode
   //    lights off
   //    heater minium 8 degrees
   switch (mode) {
     case 1: // standby
       // ensure all lights are off
       main_light_should_be_on = should_light_be_on(false, override_main_light);
       left_lamp_should_be_on = should_light_be_on(false, override_left_lamp);
       right_lamp_should_be_on = should_light_be_on(false, override_right_lamp);
       door_light_active = true;
       desired_temperature = 8;
       break;
     case 2: // all on
       main_light_should_be_on = should_light_be_on(true, override_main_light);
       left_lamp_should_be_on = should_light_be_on(true, override_left_lamp);
       right_lamp_should_be_on = should_light_be_on(true, override_right_lamp);
       door_light_active = false;
       desired_temperature = 20;
       break;
     case 3: // sleep
       main_light_should_be_on = should_light_be_on(false, override_main_light);
       left_lamp_should_be_on = should_light_be_on(false, override_left_lamp);
       right_lamp_should_be_on = should_light_be_on(false, override_right_lamp);
       desired_temperature = 18;
       door_light_active = false;
       break;
     case 4: // tv
       door_light_active = false;
       main_light_should_be_on = should_light_be_on(false, override_main_light);
       left_lamp_should_be_on = should_light_be_on(true, override_left_lamp);
       right_lamp_should_be_on = should_light_be_on(true, override_right_lamp);
       desired_temperature = 20;
       break;
     default:
     break;
   }

   // set lights to desired states
   main_light.set(main_light_should_be_on);
   left_lamp.set(left_lamp_should_be_on);
   right_lamp.set(right_lamp_should_be_on);

   check_door_light(door_light_active);

   if (stringComplete) {
     Serial.println("got input string");
     if (inputString == sleep_state_ctrl) {
       change_state(3);
     } else if (inputString == standby_state_ctrl) {
       change_state(1);
     } else if (inputString == all_on_state_ctrl) {
       change_state(2);
     } else if (inputString == tv_state_ctrl) {
       change_state(4);
     } else if (inputString == main_ctrl_on) {
       override_main_light = OVERRIDE_ON;
     } else if (inputString == main_ctrl_off) {
       override_main_light = OVERRIDE_OFF;
     } else if (inputString == right_lamp_ctrl_on) {
       override_right_lamp = OVERRIDE_ON;
     } else if (inputString == right_lamp_ctrl_off) {
       override_right_lamp = OVERRIDE_OFF;
     } else if (inputString == left_lamp_ctrl_on) {
       override_left_lamp = OVERRIDE_ON;
     } else if (inputString == left_lamp_ctrl_off) {
       override_left_lamp = OVERRIDE_OFF;
     } else if (inputString == heater_boost_ctrl) {
       heater_boost = true;
       heater_boost_enabled_ms = millis();
     }
     inputString = "";
     stringComplete = false;
   }

     // check the last time we ran the heater code
   if (millis() - last_temp_check > 3000) {
     // it's been more than 3 seconds 
     int chk = DHT.read11(dht_pin);
     Serial.print("TMP");
     Serial.println(DHT.temperature);

     if (!heater_on) {
       if ((DHT.temperature < desired_temperature) or (heater_boost && (millis() - heater_boost_enabled_ms < heater_boost_ms))) {
         Serial.println("HTRON");
         digitalWrite(heater_pin, LOW);
         heater_on = true;
         heater_on_ms = millis();
       }
     } else {
       if ((DHT.temperature > (desired_temperature + 1)) and !heater_boost) {
         Serial.println("HTROFF");
         digitalWrite(heater_pin, HIGH);
         heater_on = false;
       }
     }
     last_temp_check = millis(); 
     if (heater_boost && (millis() - heater_boost_enabled_ms > heater_boost_ms)) {
       // heater boost time has expired
       heater_boost = false;  
     }
   }   
}

void serialEvent() {
  Serial.println("serialEvent");
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

void serialEventRun(void) {
  if (Serial.available()) serialEvent();
}
