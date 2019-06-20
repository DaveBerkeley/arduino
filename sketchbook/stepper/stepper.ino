
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
    { 1, 1, 0, 0 },
    { 0, 1, 0, 0 },
    { 0, 1, 1, 0 },
    { 0, 0, 1, 0 },
    { 0, 0, 1, 1 },
    { 1, 0, 0, 1 },
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
        //Serial.print("reset\r\n");
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

static int sensor_0 = 6, sensor_1 = 7;
static CLI cli;

void setup () {
    Serial.begin(9600);
    pinMode(sensor_0, INPUT);
    pinMode(sensor_1, INPUT);
}

void loop() {
    static unsigned long elapsed = 0;
    const unsigned long now = millis();

    if (now < elapsed)
    {
        //  wrap around
        elapsed = 0;
    }

    const unsigned long tdiff = now - elapsed;

    if (tdiff >= 500)
    {
        //  reporting interval
        elapsed = now;
#if 1
        Serial.print(stepper.ready() ? 'R' : 'X');
        Serial.print(digitalRead(sensor_0) ? 'H' : 'L');
        Serial.print(digitalRead(sensor_1) ? 'H' : 'L');
        Serial.print(stepper.position());
        Serial.print("\r\n");
#endif
    }

    stepper.poll();

    if (Serial.available())
    {
        cli.process(Serial.read());
    }
}

// FIN
