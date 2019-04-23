

// LIBRARIES //
#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include <HX711_ADC.h>
#include <Adafruit_RGBLCDShield.h>
#include <DebounceInput.h>

// CONFIG FLAGS //
#define DEBUG
//#define LCD
//#define SD_CARD

// CONFIGS
const unsigned int BAUDRATE = 9600;
const unsigned long DELAY = 1000;
const unsigned int READ_TIME = 1000;
const unsigned int DEBOUNCE = 500;
unsigned long LAST_UPDATE;

// PIN ASSIGNMENTS //
const byte MASTER_START_PIN = 7; // master button pin to start test
const byte ANEMOMETER_PIN = A1; // analog input, linear range, wind-speed[m/s] = 20*input[V] - 7.6
const byte LOAD_CELL_DOUT_PIN = 4; // digital pin used to read shifted data from Load Cell
const byte LOAD_CELL_SCK_PIN = 5; //digital serial clock pin used to shift data from Load Cell
const byte HALL_SENSOR_PIN = 2 ;//A3; // analog pin scanned constantly to time rpm of shaft
const byte SD_CHIPSELECT_PIN = 10; // chipselect pin for datalogging to SD shield
const byte SD_CHIP_ERROR_PIN = 11; // digital pin connected to SD error LED
const byte SERIAL_ERROR_PIN = 12; // digital pin connected to Serial error LED

// DATA CONFIGS //
const byte ANEMOMETER_BUFFER_SIZE = 8; // size of anemometer ADC value buffer (uint)
const byte LOAD_CELL_BUFFER_SIZE = 8; // size of load cell serial data buffer (float)
const byte HALL_SENSOR_BUFFER_SIZE = 8; // size of hall sensor cycle time buffer (long)

// DATA STRUCTURES //
const byte BUFFER_SIZE = 8;
unsigned int ANEMOMETER_BUFFER[BUFFER_SIZE]; // circular buffer for anemometer data
float LOAD_CELL_BUFFER[BUFFER_SIZE]; // circular buffer for laod cell data
unsigned long HALL_SENSOR_BUFFER[BUFFER_SIZE]; // circular buffer for hall sensor data

// DATA SUPPORT //
byte ANEMOMETER_INDEX = 0;
byte LOAD_CELL_INDEX = 0;
byte HALL_SENSOR_INDEX = 0;
volatile unsigned long HALL_SENSOR_TIME = 0;

// LOAD CELL 
HX711_ADC LoadCell(LOAD_CELL_DOUT_PIN, LOAD_CELL_SCK_PIN);
const unsigned int LOAD_CELL_STABALIZING_TIME = 2000;
const float LOAD_CELL_CAL_FACTOR = 18.75;

// DATALOGGER
#ifdef SD_CARD
File DATAFILE;
String FILENAME;
#endif

// LCD (optional)
#ifdef LCD
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();
bool LCD_STATUS = true;
const byte LCD_ON = 0xF;
const byte LCD_OFF = 0;
#endif

DebouncedInput pin;

// SETUP
void setup() {

  bool error = false;

  #ifdef DEBUG
  Serial.begin(BAUDRATE);
  while(!Serial){
    digitalWrite(error, SERIAL_ERROR_PIN);
    delay(500);
    error = !error;
    Serial.begin(BAUDRATE);
  }
  #endif
  
  #ifdef LCD
  lcd.begin(16,2);
  lcd.setBacklight(LCD_ON);
  lcd.clear();
  print_lines("INITIALIZING", "BUTTONS & PINS");
  #endif
  
  init_buffers();

  pinMode(ANEMOMETER_PIN, INPUT);
  pinMode(HALL_SENSOR_PIN, INPUT_PULLUP);
  pin.attach(HALL_SENSOR_PIN);
  
  pinMode(SD_CHIP_ERROR_PIN, OUTPUT);
  pinMode(SERIAL_ERROR_PIN, OUTPUT);

  #ifdef LCD
  print_lines("INITIALIZATION", "COMPLETE!");
  print_lines("CALIBRATING", "LOAD CELL");
  #endif
  
  LoadCell.begin();
  LoadCell.start(LOAD_CELL_STABALIZING_TIME);
  LoadCell.setCalFactor(LOAD_CELL_CAL_FACTOR);
  LoadCell.tare();
 
  #ifdef LCD
  print_lines("CALIBRATION", "COMPLETE");
  print_lines("BOOTING", "DATALOGGER");
  #endif


  #ifdef SD_CARD
  if(!SD.begin(SD_CHIPSELECT_PIN)){
    #ifdef LCD
    print_lines("ERROR OPENING", "SD CARD");
    #endif
    while(!SD.begin(SD_CHIPSELECT_PIN)){
      digitalWrite(error, SD_CHIP_ERROR_PIN);
      delay(500);
      error = !error;
    }
  }
  #endif
  
  #ifdef LCD
  print_lines("BOOT COMPLETE", "SD CARD FOUND!");
  print_lines("FINDING OPEN", "FILENAME");
  #endif

  #ifdef SD_CARD
  FILENAME = find_open_filename();
  if(SD.exists(FILENAME)){
      randomSeed(analogRead(0));
      long rand_num = random(1000);
      FILENAME.setCharAt(5, (rand_num/100)%10);
      FILENAME.setCharAt(6, (rand_num/10)%10);
      FILENAME.setCharAt(7, (rand_num)%10);
  }
  #endif
  
  
  #ifdef LCD
  print_lines("OPENING FILE:", FILENAME);
  #endif

  #ifdef SD_CARD
  DATAFILE = SD.open(FILENAME, FILE_WRITE);
  if(!DATAFILE){
    #ifdef LCD
    print_lines("UNABLE TO OPEN", FILENAME);
    #endif
    while(!DATAFILE){
      digitalWrite(error, SD_CHIP_ERROR_PIN);
      delay(200);
      error = !error;
      DATAFILE = SD.open(FILENAME, FILE_WRITE);
    }
  }
  #endif
  
  #ifdef LCD
  print_lines("FILE CREATED:", FILENAME);
  #endif

  /*#ifndef LCD
  while(!digitalRead(MASTER_START_PIN));
  #endif

  #ifdef LCD
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("PRESS ANY BUTTON");
  lcd.setCursor(0,1);
  lcd.print("TO START TEST...");
  while(!lcd.readButtons()||!digitalRead(MASTER_START_PIN));
  lcd.clear();
  #endif*/
  
  LAST_UPDATE = millis();

  attachInterrupt(digitalPinToInterrupt(HALL_SENSOR_PIN), read_hall_sensor, RISING);
}

// MAIN LOOP
void loop() {
  
 read_anemometer();
 read_load_cell();
 LoadCell.update();
 

 if(millis()-LAST_UPDATE > DELAY){

  #ifdef LCD
  if(lcd.readButtons()){
    if(LCD_STATUS){
      lcd.setBacklight(LCD_OFF);
      LCD_STATUS = false;
    }
    else{
      lcd.setBacklight(LCD_ON);
      LCD_STATUS = true;
    }
  }
  #endif
  
  //detachInterrupt(digitalPinToInterrupt(HALL_SENSOR_PIN));
  
  float v = velocity();
  double f = force();
  float w = omega();
  
  #ifdef SD_CARD
  DATAFILE.print(v);
  DATAFILE.print(',');
  DATAFILE.print(f);
  DATAFILE.print(',');
  DATAFILE.print(w);
  #ifdef DEBUG
  DATAFILE.print(',');
  DATAFILE.print(millis());
  #endif
  DATAFILE.print('\n');
  #endif
  
  #ifdef LCD
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("V ");
  lcd.print(v);
  lcd.print(" m/s");
  lcd.setCursor(0,1);
  lcd.print("F ");
  lcd.print(f);
  lcd.print(" N");
  lcd.setCursor(8,1);
  lcd.print("W ");
  lcd.print(w);
  lcd.print(" /s");
  #endif
  
  #ifdef DEBUG
  Serial.print(v);
  Serial.print(',');
  Serial.print(f);
  Serial.print(',');
  Serial.print(w);
  Serial.print(',');
  Serial.print(millis());
  Serial.print('\n');
  #endif
  
  LAST_UPDATE = millis();
  
  attachInterrupt(digitalPinToInterrupt(HALL_SENSOR_PIN), read_hall_sensor, RISING);
 }
 delay(50);
}

void init_buffers(){
  for(byte i = 0; i < BUFFER_SIZE; i++){
    ANEMOMETER_BUFFER[i] = 0;
    LOAD_CELL_BUFFER[i] = 0;
    HALL_SENSOR_BUFFER[i] = 0xFFFFFFFF;
  }
}

String find_open_filename() {
  String filename = "test_001.txt";
  unsigned int file_number = 1;
  while(SD.exists(filename)) {
    file_number++;
    if(file_number == 1000){
      return "_______.txt"; 
    }
    filename.setCharAt(5, '0'+(file_number/100) % 10);
    filename.setCharAt(6, '0'+(file_number/10) % 10);
    filename.setCharAt(7, '0'+(file_number) % 10);
  }
  return filename;
}

void read_anemometer(){
  ANEMOMETER_BUFFER[ANEMOMETER_INDEX++] = analogRead(ANEMOMETER_PIN);
  ANEMOMETER_INDEX %= BUFFER_SIZE;
}

double velocity(){
  double average = 0;
  for(byte i = 0; i < BUFFER_SIZE; i++){
    average += ANEMOMETER_BUFFER[i];
  }
  average /= BUFFER_SIZE;
  // Wind Speed = 20*(average*(5/1024)) - 7.6 [m/s]
  return ((((5*average)/1024)*20)-7.6);
}

void read_load_cell(){
  LOAD_CELL_BUFFER[LOAD_CELL_INDEX++] = LoadCell.getData();
  LOAD_CELL_INDEX %= BUFFER_SIZE;
}

double force(){
  double average = 0;
  for(byte i = 0; i < BUFFER_SIZE; i++){
    average += LOAD_CELL_BUFFER[i];
  }
  average /= BUFFER_SIZE;
  // Force = 9.81*(average/1000) [N]
  return ((9.81*average)/1000);
}

void read_hall_sensor(){
  unsigned long now = millis();
  if(now-HALL_SENSOR_TIME > DEBOUNCE)
  {
      HALL_SENSOR_BUFFER[HALL_SENSOR_INDEX++] = millis() - HALL_SENSOR_TIME;
      HALL_SENSOR_INDEX %= BUFFER_SIZE;
      HALL_SENSOR_TIME = millis();
      Serial.print("INT");
  }
  
}

double omega(){
  double average = 0;
  for(byte i = 0; i < BUFFER_SIZE; i++){
    average += HALL_SENSOR_BUFFER[i];
  }
  average /= BUFFER_SIZE;
  // Omega = (2*PI/(average/1000)) [rad/s]
  return (2*PI/(average/1000));
}

#ifdef LCD
void print_lines(String line_1, String line_2){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(line_1);
  lcd.setCursor(0,1);
  lcd.print(line_2);
  delay(READ_TIME);
}
#endif
