int torque = 0;
int rotation = 0;
int wind = 0;
bool state = false;

void setup() {

  Serial.begin(9600);
  while(!Serial);

  pinMode(LED_BUILTIN, OUTPUT);
  
  randomSeed(analogRead(0));
}

void loop() {
  
  ++wind %= 26;
  torque = random(wind, 2*wind);
  rotation = random(wind, 2*wind);

  delay(500);
  
  print_values(torque, rotation, wind);

  torque = random(0, 2*wind);
  rotation = random(2*wind-torque, 2*wind);

  delay(500);

  print_values(torque, rotation, wind);

  rotation = random(0, 2*wind);
  torque = random(2*wind-rotation, 2*wind);

  delay(500);

  print_values(torque, rotation, wind);

  state = !state;
  digitalWrite(LED_BUILTIN, state);
}

void print_values(int torque, int rotation, int wind){
  Serial.print(torque);
  Serial.print(',');
  Serial.print(rotation);
  Serial.print(',');
  Serial.print(wind);
  Serial.print('\n');
}

