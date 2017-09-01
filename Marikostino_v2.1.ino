#include <SoftwareSerial.h>
#include <TinyGPS.h>
#include <Wire.h>
#include <RealTimeClockDS1307.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT_U.h>
#include "lastSunday.h"
#include <EEPROM.h>

//#include <MemoryFree.h> use that for testing the ram memory of arduino if needed

//TODO: 1) Arduino maybe freeze because of the Wire connection (I2C) because the RTC uses it
//      2) Make a timeout of 30 sec to stop the setting mode if no changes are made 

// GPS
TinyGPS gps;
SoftwareSerial gps_connection(5,4); // 5(rx) and 4(tx)

// Real Time Clock
// not really sure what those two are doing
#define Display_Clock_Every_N_Seconds 10 
#define Display_ShortHelp_Every_N_Seconds 60

// Temperature sensor
#define TEMP_WIRE_BUS 3  //temperature pin(3)
OneWire temp_wire(TEMP_WIRE_BUS);
DallasTemperature temp_sensor(&temp_wire);

// DHT11 sensor
#define DHTPIN 6      // Pin which is connected to the DHT sensor.
#define DHTTYPE DHT11 // DHT 11 
DHT_Unified dht(DHTPIN, DHTTYPE);

// Water temp
int water_temp;
int water_red_interrupt_pin = 0; // pin(2) used for the interrupt signal, if using pin(3) then set to 1
int water_temp_up_button = 12;
int water_temp_down_button = 11;
int water_temp_set_button = 2; // used for moving to the next digit
volatile boolean to_set_water_temp = false;

// Display Pins
const int clock_pin = 8;
const int latch_pin = 9;
const int data_pin = 10;

// Digits Display
byte display_digits[] = {
  B11111101, // 0
  B01100001, // 1
  B11011011, // 2
  B11110011, // 3
  B01100111, // 4
  B10110111, // 5
  B10111111, // 6
  B11100001, // 7
  B11111111, // 8
  B11110111  // 9
};
byte display_month_digit_1, display_month_digit_2, display_day_digit_1, display_day_digit_2, 
     display_hour_digit_1, display_hour_digit_2, display_minute_digit_1, display_minute_digit_2;
byte display_normal_temp_digit_1, display_normal_temp_digit_2, display_normal_temp_digit_3; // normal_temp_digit_3 is for minus sign
byte display_dht_temp_digit_1, display_dht_temp_digit_2, display_dht_hum_digit_1, display_dht_hum_digit_2;
byte display_water_temp_digit_1, display_water_temp_digit_2;


void setup() {
  Serial.begin(115200);
  gps_connection.begin(9600);
  temp_sensor.begin();
  RTC.set24h(); // set RTC into 24h mode

  // Water Temp settings
  water_temp = EEPROM.read(0);
  if (water_temp < 0 || water_temp > 99) {
    water_temp = 0;
    EEPROM.write(0, water_temp);
  }
  attachInterrupt(water_red_interrupt_pin, set_water_temp, LOW);

  // Water temp buttons
  pinMode(water_temp_up_button, INPUT);
  pinMode(water_temp_down_button, INPUT);
  pinMode(water_temp_set_button, INPUT);

  // RTC pins
  pinMode(A4, OUTPUT);
  digitalWrite(A4, HIGH);
  pinMode(A5, OUTPUT);
  digitalWrite(A5, LOW);
  
  // Display Pins
  pinMode(clock_pin, OUTPUT);
  pinMode(latch_pin, OUTPUT);
  pinMode(data_pin, OUTPUT);

  // Arduino in-built Led using for detecting halting
  pinMode(13, OUTPUT); 

  test_display();
}

void loop() {
  digitalWrite(13, HIGH);

  fetch_gps_data_and_adjust_rtc_time();
  display_rtc_time();
  
  int temp = fetch_normal_temp();
  display_normal_temp(temp);
  
  int dht_temp = fetch_dht_temperature();
  display_dht_temperature(dht_temp);
  
  int dht_hum = fetch_dht_humidity();
  display_dht_humidity(dht_hum);

  if (to_set_water_temp) {
    set_water_temp_digits();
  }

  display_water_temp(water_temp);

//  Serial.print("freeMemory= ");
//  Serial.println(freeMemory()); 

  digitalWrite(13, LOW);

  delay(100);
}


bool newData = false;
unsigned long chars;
unsigned short sentences, failed;
void fetch_gps_data_and_adjust_rtc_time() {
  bool newData = false;
  unsigned long chars;
  unsigned short sentences, failed;

  // For one second we parse GPS data and report some key values
  for (unsigned long start = millis(); millis() - start < 1000;)
  {
    while (gps_connection.available())
    {
      char c = gps_connection.read();
      if (gps.encode(c)) // Did a new valid sentence come in?
        newData = true;
    }
  }

  if (newData)
  {
    int gps_year;
    byte gps_month, gps_day, gps_hour, gps_minute, gps_second, gps_hundredths;
    unsigned long gps_age;
    gps.crack_datetime(&gps_year, &gps_month, &gps_day, &gps_hour, &gps_minute, &gps_second, &gps_hundredths, &gps_age);

    // Set RTC time
    if ((gps_hour >= 0 && gps_hour <= 24)  && (gps_minute >= 0 && gps_minute <= 60)) { // validate hour and minute
      int corrected_gps_hour = gps_hour + correct_gps_time(gps_year, gps_month, gps_day); // correct gps hour according to bulgarian time
      RTC.setHours(corrected_gps_hour);
      RTC.setMinutes(gps_minute);
      RTC.setSeconds(gps_second);
      RTC.setDate(gps_day);
      RTC.setMonth(gps_month);
      RTC.setYear(gps_year);
    }
  }  
}

void display_rtc_time() {
  RTC.readClock();
  
  int rtc_month = RTC.getMonth();
  int month_digit_1 = rtc_month / 10;
  int month_digit_2 = rtc_month % 10;
  display_month_digit_1 = display_digits[month_digit_1];
  display_month_digit_2 = display_digits[month_digit_2];

  int rtc_day = RTC.getDate();
  int day_digit_1 = rtc_day / 10;
  int day_digit_2 = rtc_day % 10;
  display_day_digit_1 = display_digits[day_digit_1];
  display_day_digit_2 = display_digits[day_digit_2];
  
  int rtc_hour = RTC.getHours();
  int hour_digit_1 = rtc_hour / 10;
  int hour_digit_2 = rtc_hour % 10;
  display_hour_digit_1 = display_digits[hour_digit_1];
  display_hour_digit_2 = display_digits[hour_digit_2];

  int rtc_minute = RTC.getMinutes();
  int minute_digit_1 = rtc_minute / 10;
  int minute_digit_2 = rtc_minute % 10;
  display_minute_digit_1 = display_digits[minute_digit_1];
  display_minute_digit_2 = display_digits[minute_digit_2];

  display_all_digits();
}

int normal_temp;
int fetch_normal_temp() {
  temp_sensor.requestTemperatures();
  int current_temp = temp_sensor.getTempCByIndex(0);
  Serial.print("Current Temp: ");
  Serial.println(current_temp);

  // validate data if temperature is broken
  if (normal_temp < -25) {
    int fix_this; 
  } else if (normal_temp > 45) {
    int fix_this2;
  } else {
    normal_temp = current_temp;
  }

  return normal_temp;
}

void display_normal_temp(int normal_temp) {
  if (normal_temp < 0) { // if temperature is negative
    normal_temp = abs(normal_temp);
    int normal_temp_digit_1 = normal_temp / 10; //taking first digit of the number
    int normal_temp_digit_2 = normal_temp % 10; //taking second digit of the number
    //finding the correct number in digits table 
    display_normal_temp_digit_1  = display_digits[normal_temp_digit_1];
    display_normal_temp_digit_2 = display_digits[normal_temp_digit_2];
    display_normal_temp_digit_3 = B00000010;
  } 
  else if (normal_temp == 0) { // if temp is 0
    display_normal_temp_digit_1 = B00000000;
    display_normal_temp_digit_2 = B11111100;
    display_normal_temp_digit_3 = B00000000;
  }
  else { // if temp is positive
      int normal_temp_digit_1 = normal_temp / 10;
      int normal_temp_digit_2 = normal_temp % 10;
      display_normal_temp_digit_1 = display_digits[normal_temp_digit_1];
      display_normal_temp_digit_2 = display_digits[normal_temp_digit_2];
      display_normal_temp_digit_3 = B00000000;
  }

  display_all_digits();
  
  Serial.print("Normal Temp is: ");
  Serial.println(normal_temp);
}

int fetch_dht_temperature() {
  sensors_event_t event;  
  dht.temperature().getEvent(&event);
  int dht_temp = event.temperature;

  return dht_temp;
}

void display_dht_temperature(int dht_temp) {
  int dht_temp_digit_1 = dht_temp / 10;
  int dht_temp_digit_2 = dht_temp % 10;
  display_dht_temp_digit_1 = display_digits[dht_temp_digit_1];
  display_dht_temp_digit_2 = display_digits[dht_temp_digit_2];

  display_all_digits();
  
  Serial.print("DHT temp is: ");
  Serial.println(dht_temp);
}

int fetch_dht_humidity() {
  sensors_event_t event;
  dht.humidity().getEvent(&event);
  int dht_hum = event.relative_humidity; 

  return dht_hum;
}

void display_dht_humidity(int dht_hum) {
  int dht_hum_digit_1 = dht_hum / 10;
  int dht_hum_digit_2 = dht_hum % 10;
  display_dht_hum_digit_1 = display_digits[dht_hum_digit_1];
  display_dht_hum_digit_2 = display_digits[dht_hum_digit_2];

  display_all_digits();
  
  Serial.print("DHT hum is: ");
  Serial.println(dht_hum);
}

void display_water_temp(int water_temp) {
  int water_temp_digit_1 = water_temp / 10;
  int water_temp_digit_2 = water_temp % 10;
  display_water_temp_digit_1 = display_digits[water_temp_digit_1];
  display_water_temp_digit_2 = display_digits[water_temp_digit_2];

  display_all_digits();

//  Serial.print("Water temp is: ");
//  Serial.println(water_temp);
}

void display_all_digits() {
  digitalWrite(latch_pin, LOW); 

  shiftOut(data_pin, clock_pin, LSBFIRST, display_normal_temp_digit_3);
  shiftOut(data_pin, clock_pin, LSBFIRST, display_normal_temp_digit_1);
  shiftOut(data_pin, clock_pin, LSBFIRST, display_normal_temp_digit_2);

  shiftOut(data_pin, clock_pin, LSBFIRST, display_water_temp_digit_2); 
  shiftOut(data_pin, clock_pin, LSBFIRST, display_water_temp_digit_1);

  shiftOut(data_pin, clock_pin, LSBFIRST, display_dht_temp_digit_1);
  shiftOut(data_pin, clock_pin, LSBFIRST, display_dht_temp_digit_2);

  shiftOut(data_pin, clock_pin, LSBFIRST, display_dht_hum_digit_1);
  shiftOut(data_pin, clock_pin, LSBFIRST, display_dht_hum_digit_2);

  shiftOut(data_pin, clock_pin, LSBFIRST, display_month_digit_2);
  shiftOut(data_pin, clock_pin, LSBFIRST, display_month_digit_1);
  shiftOut(data_pin, clock_pin, LSBFIRST, display_day_digit_2);
  shiftOut(data_pin, clock_pin, LSBFIRST, display_day_digit_1);
  
  shiftOut(data_pin, clock_pin, LSBFIRST, display_minute_digit_2);
  shiftOut(data_pin, clock_pin, LSBFIRST, display_minute_digit_1);
  shiftOut(data_pin, clock_pin, LSBFIRST, display_hour_digit_2);
  shiftOut(data_pin, clock_pin, LSBFIRST, display_hour_digit_1);
  
  digitalWrite(latch_pin, HIGH);
  digitalWrite(latch_pin, LOW);
}


lastSunday ls; // libary used for correcting the gps hour time (UTF) and making it to bulgarian time
int correct_gps_time(int gps_year, int gps_month, int gps_day) {
  //find every last sunday of each month of selected year
  ls.findLastSunday(gps_year);
  
  //getting the last sunday of selected month
  //months start from 0
  int last_sun_mar = ls.getDay(2);
  int last_sun_oct = ls.getDay(9);
 
  //checking what month and day it is and determining whether to add +2 or +3 to UTC time 
  bool state1 = ((gps_month == 10) && (gps_day >= last_sun_oct));
  bool state2 = ((gps_month > 10) || (gps_month <= 2));
  bool state3 = ((gps_month == 3) && (gps_day < last_sun_mar));

  if (state1 || (state2 || state3)){
    return 2;
  } else {
    return 3;
  }
}

void set_water_temp() {
  to_set_water_temp = true;
  detachInterrupt(water_red_interrupt_pin); // detach the interrupt because we are using the red button 
                                            // to change between digits
}

void set_water_temp_digits() {  
  boolean is_water_temp_set = false;
  boolean setting_digit_1 = true; // first set digit 1
  boolean setting_digit_2 = false; 
  int temp_digit_1 = water_temp / 10;
  int temp_digit_2 = water_temp % 10;
  boolean move_to_next_digit = false;
  boolean up = false;
  boolean down = false;
  unsigned long blink_timer = millis();
  int blink_interval = 200;
    
  while (! is_water_temp_set) {
    while (setting_digit_1) {
      if (move_to_next_digit) {
        setting_digit_1 = false;
        setting_digit_2 = true;
        move_to_next_digit = false;
      }

      move_to_next_digit = validate_button_press(water_temp_set_button);
      up = validate_button_press(water_temp_up_button);
      // doing if else logic to prevent if someone presses both up and down at the same time
      if (up) {
        temp_digit_1++;
        if (temp_digit_1 >= 10) {
          temp_digit_1 = 0;
        }

        water_temp = temp_digit_1 * 10 + water_temp % 10;
        display_water_temp(water_temp);
        
      } else { // check down button
        down = validate_button_press(water_temp_down_button);
        if (down) {
          temp_digit_1--;
          if (temp_digit_1 < 0) {
            temp_digit_1 = 9;
          }

          water_temp = temp_digit_1 * 10 + water_temp % 10;
          display_water_temp(water_temp);
        
        }
      }

      if (millis() - blink_timer >= blink_interval) {
        blink_timer = millis();
        blink_water_temp_digit(1);
      }
    }

    while (setting_digit_2) {
      if (move_to_next_digit) {
        setting_digit_1 = false;
        setting_digit_2 = false;
        move_to_next_digit = false;
      }

      
      move_to_next_digit = validate_button_press(water_temp_set_button);
      up = validate_button_press(water_temp_up_button);
      // doing if else logic to prevent if someone presses both up and down at the same time
      if (up) {
        temp_digit_2++;
        if (temp_digit_2 >= 10) {
          temp_digit_2 = 0;
        }
        water_temp = (water_temp / 10) * 10 + temp_digit_2;        
        display_water_temp(water_temp);
      } else { // check down button
        down = validate_button_press(water_temp_down_button);
        if (down) {
          temp_digit_2--;
          if (temp_digit_2 < 0) {
            temp_digit_2 = 9;
          }
          water_temp = (water_temp / 10) * 10 + temp_digit_2;
          display_water_temp(water_temp);
        }
      }

      if (millis() - blink_timer >= blink_interval) {
        blink_timer = millis();
        blink_water_temp_digit(2);
      }
    }

    if (!setting_digit_1 && !setting_digit_2) { // all digits are set
      display_water_temp(water_temp);
      is_water_temp_set = true;
      to_set_water_temp = false;
      EEPROM.write(0, water_temp); // save water temp into memory
      
      Serial.println("Water Temp is set!");
      delay(5000); // 5 sec delay before attaching the interrupt
      attachInterrupt(water_red_interrupt_pin, set_water_temp, LOW); // attached the interrupt again
    }
  }
}

int water_temp_button_set_last_state = HIGH;
int water_temp_button_set_state = HIGH;

int water_temp_button_down_last_state = HIGH;
int water_temp_button_down_state = HIGH;

int water_temp_button_up_last_state = HIGH;
int water_temp_button_up_state = HIGH;

boolean validate_button_press(int digital_pin) {
  if (digital_pin == water_temp_set_button) { // check the water_temp_set_button
    // read the pushbutton input pin:
    water_temp_button_set_state = digitalRead(water_temp_set_button);
    // compare the buttonState to its previous state
    if (water_temp_button_set_state != water_temp_button_set_last_state) {
      if (water_temp_button_set_state == LOW) {
        // if the current state is LOW then the button went from off to on:
        // Delay a little bit to avoid bouncing
        delay(50);
        // save the current state as the last state, for next time through the loop
        water_temp_button_set_last_state = water_temp_button_set_state;
        return true;
      } else {
        // if the current state is HIGH then the button went from on to off:
        // Delay a little bit to avoid bouncing
        delay(50);
        // save the current state as the last state, for next time through the loop
        water_temp_button_set_last_state = water_temp_button_set_state;
        return false;
      }
    }
  }
  
  if (digital_pin == water_temp_down_button) { // check the water_temp_down_button
    // read the pushbutton input pin:
    water_temp_button_down_state = digitalRead(water_temp_down_button);
    // compare the buttonState to its previous state
    if (water_temp_button_down_state != water_temp_button_down_last_state) {
      if (water_temp_button_down_state == LOW) {
        // if the current state is LOW then the button went from off to on:
        // Delay a little bit to avoid bouncing
        delay(50);
        // save the current state as the last state, for next time through the loop
        water_temp_button_down_last_state = water_temp_button_down_state;
        return true;
      } else {
        // if the current state is HIGH then the button went from on to off:
        // Delay a little bit to avoid bouncing
        delay(50);
        // save the current state as the last state, for next time through the loop
        water_temp_button_down_last_state = water_temp_button_down_state;
        return false;
      }
    }
  }

  if (digital_pin == water_temp_up_button) { // check the water_temp_up_button
    // read the pushbutton input pin:
    water_temp_button_up_state = digitalRead(water_temp_up_button);
    // compare the buttonState to its previous state
    if (water_temp_button_up_state != water_temp_button_up_last_state) {
      if (water_temp_button_up_state == LOW) {
        // if the current state is LOW then the button went from off to on:
        // Delay a little bit to avoid bouncing
        delay(50);
        // save the current state as the last state, for next time through the loop
        water_temp_button_up_last_state = water_temp_button_up_state;
        return true;
      } else {
        // if the current state is HIGH then the button went from on to off:
        // Delay a little bit to avoid bouncing
        delay(50);
        // save the current state as the last state, for next time through the loop
        water_temp_button_up_last_state = water_temp_button_up_state;
        return false;
      }
    }
  }
  
  return false; // default
}

unsigned long blink_counter = 0;
void blink_water_temp_digit(int digit) {
  digitalWrite(latch_pin, LOW); 

  shiftOut(data_pin, clock_pin, LSBFIRST, display_normal_temp_digit_3);
  shiftOut(data_pin, clock_pin, LSBFIRST, display_normal_temp_digit_1);
  shiftOut(data_pin, clock_pin, LSBFIRST, display_normal_temp_digit_2);

  if (digit == 1) {
    if (blink_counter % 2 == 0) {
      shiftOut(data_pin, clock_pin, LSBFIRST, display_water_temp_digit_2);
      shiftOut(data_pin, clock_pin, LSBFIRST, B0000000);
      blink_counter++;
    } else {
      shiftOut(data_pin, clock_pin, LSBFIRST, display_water_temp_digit_2);
      shiftOut(data_pin, clock_pin, LSBFIRST, display_water_temp_digit_1);
      blink_counter++;
    }
  }
  else if (digit == 2) {
    if (blink_counter % 2 == 0) {
      shiftOut(data_pin, clock_pin, LSBFIRST, B0000000);
      shiftOut(data_pin, clock_pin, LSBFIRST, display_water_temp_digit_1);
      blink_counter++;
    } else {
      shiftOut(data_pin, clock_pin, LSBFIRST, display_water_temp_digit_2);
      shiftOut(data_pin, clock_pin, LSBFIRST, display_water_temp_digit_1);
      blink_counter++;
    }
  }
  
  shiftOut(data_pin, clock_pin, LSBFIRST, display_dht_temp_digit_1);
  shiftOut(data_pin, clock_pin, LSBFIRST, display_dht_temp_digit_2);

  shiftOut(data_pin, clock_pin, LSBFIRST, display_dht_hum_digit_1);
  shiftOut(data_pin, clock_pin, LSBFIRST, display_dht_hum_digit_2);

  shiftOut(data_pin, clock_pin, LSBFIRST, display_month_digit_2);
  shiftOut(data_pin, clock_pin, LSBFIRST, display_month_digit_1);
  shiftOut(data_pin, clock_pin, LSBFIRST, display_day_digit_2);
  shiftOut(data_pin, clock_pin, LSBFIRST, display_day_digit_1);

  shiftOut(data_pin, clock_pin, LSBFIRST, display_minute_digit_2);
  shiftOut(data_pin, clock_pin, LSBFIRST, display_minute_digit_1);
  shiftOut(data_pin, clock_pin, LSBFIRST, display_hour_digit_2);
  shiftOut(data_pin, clock_pin, LSBFIRST, display_hour_digit_1);
  
  digitalWrite(latch_pin, HIGH);
  digitalWrite(latch_pin, LOW);
}

//Test all display with 8 beside minus symbol
void test_display() {
  digitalWrite(latch_pin, LOW);
  shiftOut(data_pin, clock_pin, LSBFIRST, B00000010);
  shiftOut(data_pin, clock_pin, LSBFIRST, B11111111);
  shiftOut(data_pin, clock_pin, LSBFIRST, B11111111);
 
  shiftOut(data_pin, clock_pin, LSBFIRST, B11111111);
  shiftOut(data_pin, clock_pin, LSBFIRST, B11111111);

  shiftOut(data_pin, clock_pin, LSBFIRST, B11111111);
  shiftOut(data_pin, clock_pin, LSBFIRST, B11111111);

  shiftOut(data_pin, clock_pin, LSBFIRST, B11111111);
  shiftOut(data_pin, clock_pin, LSBFIRST, B11111111);

  shiftOut(data_pin, clock_pin, LSBFIRST, B11111111);
  shiftOut(data_pin, clock_pin, LSBFIRST, B11111111);
  shiftOut(data_pin, clock_pin, LSBFIRST, B11111111);
  shiftOut(data_pin, clock_pin, LSBFIRST, B11111111);
  
  shiftOut(data_pin, clock_pin, LSBFIRST, B11111111);
  shiftOut(data_pin, clock_pin, LSBFIRST, B11111111);
  shiftOut(data_pin, clock_pin, LSBFIRST, B11111111);
  shiftOut(data_pin, clock_pin, LSBFIRST, B11111111);
  digitalWrite(latch_pin, HIGH);
  digitalWrite(latch_pin, LOW);

  delay(1000);

  digitalWrite(latch_pin, LOW);
  shiftOut(data_pin, clock_pin, LSBFIRST, B00000010);
  shiftOut(data_pin, clock_pin, LSBFIRST, B11111111);
  shiftOut(data_pin, clock_pin, LSBFIRST, B11111111);
 
  shiftOut(data_pin, clock_pin, LSBFIRST, B11111111);
  shiftOut(data_pin, clock_pin, LSBFIRST, B11111111);

  shiftOut(data_pin, clock_pin, LSBFIRST, B11111111);
  shiftOut(data_pin, clock_pin, LSBFIRST, B11111111);

  shiftOut(data_pin, clock_pin, LSBFIRST, B11111111);
  shiftOut(data_pin, clock_pin, LSBFIRST, B11111111);

  shiftOut(data_pin, clock_pin, LSBFIRST, B11111111);
  shiftOut(data_pin, clock_pin, LSBFIRST, B11111111);
  shiftOut(data_pin, clock_pin, LSBFIRST, B11111111);
  shiftOut(data_pin, clock_pin, LSBFIRST, B11111111);
  
  shiftOut(data_pin, clock_pin, LSBFIRST, B11111111);
  shiftOut(data_pin, clock_pin, LSBFIRST, B11111111);
  shiftOut(data_pin, clock_pin, LSBFIRST, B11111111);
  shiftOut(data_pin, clock_pin, LSBFIRST, B11111111);
  digitalWrite(latch_pin, HIGH);
  digitalWrite(latch_pin, LOW);

  delay(500);
}



