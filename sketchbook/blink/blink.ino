
#define LED 6

void setup()
{
  Serial.begin(57600);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
}

void loop()
{
  static bool on = true;
  
  digitalWrite(LED, on ? LOW : HIGH);
  on = !on;
  delay(200);

  Serial.print(".");
}
