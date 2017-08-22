#include <SoftwareSerial.h>
#include <TinyGPS.h>
#include <Wire.h>
#include "RTClib.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT_U.h>
#include "lastSunday.h"


#include <MemoryFree.h>

// GPS
TinyGPS gps;
SoftwareSerial gps_connection(5,4); // 5(rx) and 4(tx)

// Real Time Clock
RTC_DS1307 rtc;

// Temperature sensor
#define TEMP_WIRE_BUS 2  //temperature pin(2)
OneWire temp_wire(TEMP_WIRE_BUS);
DallasTemperature temp_sensor(&temp_wire);

//DHT11 sensor
#define DHTPIN 6      // Pin which is connected to the DHT sensor.
#define DHTTYPE DHT11 // DHT 11 
DHT_Unified dht(DHTPIN, DHTTYPE);

// Display Pins
const int clock_pin = 8;
const int latch_pin = 9;
const int data_pin = 10;

// Digits Display
byte display_digits[] = {
  B11111100, // 0
  B01100000, // 1
  B11011010, // 2
  B11110010, // 3
  B01100110, // 4
  B10110110, // 5
  B10111110, // 6
  B11100000, // 7
  B11111110, // 8
  B11110110  // 9
};
byte display_month_digit_1, display_month_digit_2, display_day_digit_1, display_day_digit_2, 
     display_hour_digit_1, display_hour_digit_2, display_minute_digit_1, display_minute_digit_2;
byte display_normal_temp_digit_1, display_normal_temp_digit_2, display_normal_temp_digit_3; // normal_temp_digit_3 is for minus sign
byte display_dht_temp_digit_1, display_dht_temp_digit_2, display_dht_hum_digit_1, display_dht_hum_digit_2;


void setup() {
  Serial.begin(115200);
  gps_connection.begin(9600);
  rtc.begin();
  temp_sensor.begin();

  // Display Pins
  pinMode(clock_pin, OUTPUT);
  pinMode(latch_pin, OUTPUT);
  pinMode(data_pin, OUTPUT);

  pinMode(13, OUTPUT);

//  rtc.adjust(DateTime(2000, 1, 1, 1, 1, 1));
//  display_rtc_time();

  test_display();
}

void loop() {
  digitalWrite(13, HIGH);
  // put your main code here, to run repeatedly:
  fetch_gps_data_and_adjust_rtc_time();
  display_rtc_time();
  Serial.println("11111111111111111111111111111111");
  int temp = fetch_normal_temp();
  display_normal_temp(temp);
  Serial.println("22222222222222222222222222222222");
  int dht_temp = fetch_dht_temperature();
  display_dht_temperature(dht_temp);
  Serial.println("33333333333333333333333333333333");
  int dht_hum = fetch_dht_humidity();
  display_dht_humidity(dht_hum);
  Serial.println("44444444444444444444444444444444");
  Serial.print("freeMemory= ");
  Serial.println(freeMemory());

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
    Serial.println("New Data!!!");
    int gps_year;
    byte gps_month, gps_day, gps_hour, gps_minute, gps_second, gps_hundredths;
    unsigned long gps_age;
    gps.crack_datetime(&gps_year, &gps_month, &gps_day, &gps_hour, &gps_minute, &gps_second, &gps_hundredths, &gps_age);
    Serial.println("New Data 2");

    // adjust rtc time
    if ((gps_hour >= 0 && gps_hour <= 24)  && (gps_minute >= 0 && gps_minute <= 60)) { // validate hour and minute
      int corrected_gps_hour = gps_hour + correct_gps_time(gps_year, gps_month, gps_day); // correct gps hour according to bulgarian time
      rtc.adjust(DateTime(gps_year, gps_month, gps_day, corrected_gps_hour, gps_minute, gps_second));
    }
    Serial.println("New Data 3");
  }  
}

void display_rtc_time() {
  Serial.println("display1111111111111");
  Wire.endTransmission(false); // restart
  delay(500);
  if (rtc.isrunning()) {
    Serial.println("Is running?");
    Serial.print("Available bytes: ");
    Serial.println(Wire.available());
    DateTime rtc_time = rtc.now();
    Serial.println("after display1111111111111");

    if (rtc_time.hour() < 25) { // validate time
      Serial.print(rtc_time.year(), DEC);
      Serial.print('/');
      Serial.print(rtc_time.month(), DEC);
      Serial.print('/');
      Serial.print(rtc_time.day(), DEC);
      Serial.println("");
      Serial.print(rtc_time.hour(), DEC);
      Serial.print(':');
      Serial.print(rtc_time.minute(), DEC);
      Serial.print(':');
      Serial.print(rtc_time.second(), DEC);
      Serial.println();
      Serial.println("-----------------------"); 
    }
  }

  DateTime rtc_time = rtc.now();

  int rtc_month = rtc_time.month();
  int month_digit_1 = rtc_month / 10;
  int month_digit_2 = rtc_month % 10;
  display_month_digit_1 = display_digits[month_digit_1];
  display_month_digit_2 = display_digits[month_digit_2];

  int rtc_day = rtc_time.day();
  int day_digit_1 = rtc_day / 10;
  int day_digit_2 = rtc_day % 10;
  display_month_digit_1 = display_digits[day_digit_1];
  display_month_digit_2 = display_digits[day_digit_2];
  
  int rtc_hour = rtc_time.hour();
  int hour_digit_1 = rtc_hour / 10;
  int hour_digit_2 = rtc_hour % 10;
  display_hour_digit_1 = display_digits[hour_digit_1];
  display_hour_digit_2 = display_digits[hour_digit_2];

  int rtc_minute = rtc_time.minute();
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

void display_all_digits() {
  digitalWrite(latch_pin, LOW); 

  shiftOut(data_pin, clock_pin, LSBFIRST, display_normal_temp_digit_3);
  shiftOut(data_pin, clock_pin, LSBFIRST, display_normal_temp_digit_1);
  shiftOut(data_pin, clock_pin, LSBFIRST, display_normal_temp_digit_2);

  shiftOut(data_pin, clock_pin, LSBFIRST, display_digits[5]);
  shiftOut(data_pin, clock_pin, LSBFIRST, display_digits[5]);

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




