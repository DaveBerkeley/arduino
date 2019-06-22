
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

class CLI
{
    char command;
    int value;
    bool get_value;
    int sign;
    bool get_sign;

    void reset()
    {
        command = 0;
        value = 0;
        get_value = 0;
        sign = 1;
        get_sign = 0;
    }

    void run()
    {
        switch (command)
        {
            case 'G' :
                stepper.seek(value);
                break;
            case 'S' :
                stepper.set_steps(value);
                break;
            case 'Z' :
                stepper.zero();
                break;
            case 'R' :
                const int delta = value * sign;
                stepper.seek(stepper.position() + delta);
                break;
        }
    }

public:
    CLI()
    {
        reset();
    }

    void process(char c)
    {
        if ((c == '\r') || (c == '\n'))
        {
            if (command)
            {
                run();
            }
            else 
            {
                reset();
            }
            return;
        }

        if ((c >= '0') && (c <= '9'))
        {
            // numeric
            if (get_value)
            {
                value *= 10;
                value += c - '0';
            }
            else
            {
                reset();
            }
            return;
        }

        if ((c == '+') || (c == '-'))
        {
            if (get_sign)
            {
                sign = (c = '+') ? 1 : -1;
            }
            else
            {
                reset();
            }
            return;
        }

        switch (c)
        {
            case 'Z' : // zero
                command = c;
                break;
            case 'R' : // relative
                get_sign = 1;
                // fall through
            case 'S' : // steps
            case 'G' : // goto
                command = c;
                get_value = true;
                value = 0;
                break;

            default:
                reset();
                return;
        }
        return;
    }
};

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

static int sensor_0 = 6, sensor_1 = 7;
static CLI cli;

void setup () {
    Serial.begin(9600);
    pinMode(sensor_0, INPUT);
    pinMode(sensor_1, INPUT);
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
