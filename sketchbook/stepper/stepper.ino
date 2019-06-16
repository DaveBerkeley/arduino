
  /*
  *
  */

#define PINS 4

static int pins[] = { 8, 9, 10, 11 };

#define STATES 7

const static int cycle[STATES][PINS] = {
    { 1, 0, 0, 0 },
    { 1, 1, 0, 0 },
    { 0, 1, 0, 0 },
    { 0, 1, 1, 0 },
    { 0, 0, 1, 0 },
    { 0, 0, 1, 1 },
    { 1, 0, 0, 1 },
};

static void set_state(int i)
{
    const int* states = cycle[i];
    for (int i = 0; i < PINS; i++)
    {
        digitalWrite(pins[i], states[i]);
    }
}

void setup () {
    Serial.begin(57600);
    for (int i = 0; i < PINS; i++)
    {
        pinMode(pins[i], OUTPUT);
        digitalWrite(pins[i], 0);
    }
}

void loop() {
    delayMicroseconds(1000);

    static int state = 0;
    set_state(state);
    state -= 1;
    //state += 1;

    while (state < 0)
        state += STATES;
    while (state >= STATES)
        state -= STATES;
}

// FIN
