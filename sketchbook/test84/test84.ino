
#define PIN 10

static char pins[] = {
  0, 
  1,
  2,
  3, 
  7,  
  10,
  -1,
};

void setup() {
  // put your setup code here, to run once:
  for (int i = 0; ; i++)
  {
    if (pins[i] == -1)
      break;
    pinMode(pins[i], OUTPUT);    
  }
}

void loop() {
  // put your main code here, to run repeatedly:

  delay(10);

  static int state;
  for (int i = 0; ; i++) {
    if (pins[i] == -1)
      break;
    digitalWrite(pins[i], (state & (1 << i)) ? HIGH : LOW);
  }
  state += 1;
}
