
#include "cli.h"

  /*
  *
  */

class Stepper
{
    #define PINS 4
    int pins[PINS];

    #define STATES 8
    const static int cycle[STATES][PINS];

    int state;
    int steps;
    int count;
    int target;
    int period;

public:
    Stepper(int cycle, int p1, int p2, int p3, int p4, int time=1000)
    : state(0), steps(cycle), count(0), target(0), period(time)
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

        set_state(0);
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

    int get_target()
    {
        return target;
    }

    bool ready()
    {
        return target == count;
    }

    void zero()
    {
        target = 0;
        count = 0;
    }

    void set_steps(int s)
    {
        steps = s;
    }

    void poll()
    {
        const int delta = count - target;

        if (delta == 0)
            return;

        step(delta < 0);

        const int move = abs(delta);

        if (move < 5)
        {
            delayMicroseconds(5 * period);
        }
        else if (move < 20)
        {
            delayMicroseconds(2 * period);
        }
        else
        {
            delayMicroseconds(1 * period);
        }
    }
};

const int Stepper::cycle[STATES][PINS] = {
    { 1, 0, 0, 0 },
    { 1, 0, 0, 1 },
    { 0, 0, 0, 1 },
    { 0, 0, 1, 1 },
    { 0, 0, 1, 0 },
    { 0, 1, 1, 0 },
    { 0, 1, 0, 0 },
    { 1, 1, 0, 0 },
};

static Stepper stepper(4000, 8, 9, 10, 11, 1000);

    /*
    *
    */

static void on_g(char cmd, int value, void *arg)
{
    Stepper *s = (Stepper*) arg;
    s->seek(value);
}

static void on_s(char cmd, int value, void *arg)
{
    Stepper *s = (Stepper*) arg;
    s->set_steps(value);
}

static void on_z(char cmd, int value, void *arg)
{
    Stepper *s = (Stepper*) arg;
    s->zero();
}

static void on_plus(char cmd, int value, void *arg)
{
    Stepper *s = (Stepper*) arg;
    s->seek(s->get_target() + value);
}

static void on_minus(char cmd, int value, void *arg)
{
    Stepper *s = (Stepper*) arg;
    s->seek(s->get_target() + value);
}

static Action g_a = { 'G', on_g, & stepper, 0 };
static Action s_a = { 'S', on_s, & stepper, 0 };
static Action z_a = { 'Z', on_z, & stepper, 0 };
static Action p_a = { '+', on_plus, & stepper, 0 };
static Action m_a = { '-', on_minus, & stepper, 0 };

    /*
    *
    */

static void report(Stepper stepper, int sensor_0, int sensor_1)
{
    static unsigned long elapsed = 0;
    const unsigned long now = millis();

    static char last[16];
    char buff[16];

    snprintf(buff, sizeof(buff), "%c%c%c%d\r\n", 
        stepper.ready() ? 'R' : 'X',
        digitalRead(sensor_0) ? 'H' : 'L',
        digitalRead(sensor_1) ? 'H' : 'L',
        stepper.position());

    int diff = strcmp(last, buff);
    bool ready = buff[0] == 'R';

    bool do_report = false;
    if (diff && ready)
    {
        // we've become Ready, always report
        do_report = true;
    }

    if (now < elapsed)
    {
        //  wrap around
        elapsed = 0;
    }

    const unsigned long tdiff = now - elapsed;
    // higher report rate during moves
    const unsigned int period = diff ? 100 : 1000;

    if (tdiff >= period)
    {
        //  reporting interval
        do_report = true;
    }

    if (do_report)
    {
        elapsed = now;
        Serial.print(buff);
        strncpy(last, buff, sizeof(last));
    }
}

    /*
    *
    */

static int sensor_0 = 12, sensor_1 = 13;
static CLI cli;

void setup () {
    Serial.begin(9600);
    pinMode(sensor_0, INPUT);
    pinMode(sensor_1, INPUT);

    // Add handlers to CI
    cli.add_action(& g_a);
    cli.add_action(& s_a);
    cli.add_action(& z_a);
    cli.add_action(& p_a);
    cli.add_action(& m_a);
}

void loop() {
    report(stepper, sensor_0, sensor_1);

    stepper.poll();

    if (Serial.available())
    {
        cli.process(Serial.read());
    }
}

// FIN
