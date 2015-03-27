
// set pin numbers:
const int relay0 = 2;
const int relay1 = 3;
const int relay2 = 4;
const int relay3 = 5;

static const int pins[4] = { 2, 3, 4, 5 };

static void relay(int i, bool set)
{
  digitalWrite(pins[i], !set);
}

void setup()
{
  Serial.begin(57600);

  for (int i = 0; i < 4; ++i) {
    pinMode(pins[i], OUTPUT);
    relay(i, 0);
  }
}

void loop()
{
  static int i = 0;
  static int j = 0;
  
  if (++i < 10000)
    return;
  i = 0;
  if (++j < 50)
    return;
  j = 0;
  
  static int shift = 1;

  Serial.print(shift);
  Serial.print("\n");

  relay(0, shift & 0x01);
  relay(1, shift & 0x02);
  relay(2, shift & 0x04);
  relay(3, shift & 0x08);

  shift <<= 1;
  if (shift == 0x10)
    shift = 0x01;
}

// FIN
