
#define LED 6

void setup()
{
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
}

void loop()
{
  static bool on = true;
  
  digitalWrite(LED, on ? LOW : HIGH);
  on = !on;
  delay(2000);
}
