
  /*
  *
  */

class Stepper
{
    #define PINS 4
    int pins[PINS];

    #define STATES 7
    const static int cycle[STATES][PINS];

    int state;
    int steps;
    int count;
    int target;

public:
    Stepper(int cycle, int p1, int p2, int p3, int p4)
    : state(0), steps(cycle), count(0), target(0)
    {
        pins[0] = p1;
        pins[1] = p2;
        pins[2] = p3;
        pins[3] = p4;

        for (int i = 0; i < PINS; i++)
        {
            pinMode(pins[i], OUTPUT);
            digitalWrite(pins[i], 0);
        }
    }

    void set_state(int s)
    {
        const int* states = cycle[s];
        for (int i = 0; i < PINS; i++)
        {
            digitalWrite(pins[i], states[i]);
        }
        state = s;
    }

    void step(bool up)
    {
        // calculate next state
        int delta = up ? 1 : -1;
        int s = state + delta;
        if (s < 0)
            s += STATES;
        if (s >= STATES)
            s-= STATES;
        set_state(s);

        int c = count + delta;
        if (c < 0)
            c += steps;
        if (c >= steps)
            c -= steps;
        count = c;
    }

    int position()
    {
        return count;
    }

    void seek(int t)
    {
        target = t;
    }

    bool ready()
    {
        return target == count;
    }

    void poll()
    {
        if (target == count)
            return;

        step(count < target);
        delay(1);
    }
};

const int Stepper::cycle[STATES][PINS] = {
    { 1, 0, 0, 0 },
    { 1, 1, 0, 0 },
    { 0, 1, 0, 0 },
    { 0, 1, 1, 0 },
    { 0, 0, 1, 0 },
    { 0, 0, 1, 1 },
    { 1, 0, 0, 1 },
};

    /*
    *
    */

static Stepper stepper(4000, 8, 9, 10, 11);

void setup () {
    //Serial.begin(57600);
}

void loop() {
    stepper.poll();

    if (stepper.ready())
    {
        int p = random(4000);
        stepper.seek(p);
    }
}

// FIN
