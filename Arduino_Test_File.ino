#define MAX_WIND 25

int torque = 0;
int rotation = 0;
int wind = 0;
bool state = false;

void setup() {
  
  // Initialize serial communication
  Serial.begin(9600);
  while(!Serial);

  // Setup builtin LED for heartbeat
  pinMode(LED_BUILTIN, OUTPUT);
  
  // Seed the random number generator w/ floating analog input
  randomSeed(analogRead(0));
}

void loop() {
  
  // Increment wind value & keep it within range
  ++wind %= MAX_WIND+1;
  
  // Random data values in upper 1st quadrant
  torque = random(wind, 2*wind);
  rotation = random(wind, 2*wind);

  delay(500);
  
  print_values(torque, rotation, wind);

  // Random data values w/ unbounded torque
  torque = random(0, 2*wind);
  rotation = random(2*wind-torque, 2*wind);

  delay(500);

  print_values(torque, rotation, wind);

  // Random data values w/ unbounded rotation
  rotation = random(0, 2*wind);
  torque = random(2*wind-rotation, 2*wind);

  delay(500);

  print_values(torque, rotation, wind);

  // Toggle heartbeat LED
  state = !state;
  digitalWrite(LED_BUILTIN, state);
}

// CSV-style serial output of data values
void print_values(int torque, int rotation, int wind){
  Serial.print(torque);
  Serial.print(',');
  Serial.print(rotation);
  Serial.print(',');
  Serial.print(wind);
  Serial.print('\n');
}

