
#include "motor.h"
#include "cli.h"

    /*
    *
    */

static Stepper stepper(4000, 8, 9, 10, 11, 1000);
static int sensor_0 = 12, sensor_1 = 13;
static CLI cli;

    /*
    *   Command handlers
    */

static void on_g(char cmd, int value, void *arg)
{
    Stepper *s = (Stepper*) arg;
    s->seek(value);
}

static void on_r(char cmd, int value, void *arg)
{
    Stepper *s = (Stepper*) arg;
    s->rotate(value);
}

static void on_s(char cmd, int value, void *arg)
{
    Stepper *s = (Stepper*) arg;
    s->set_steps(value);
}

static void on_z(char cmd, int value, void *arg)
{
    Stepper *s = (Stepper*) arg;
    s->zero(value);
}

static void on_plus(char cmd, int value, void *arg)
{
    Stepper *s = (Stepper*) arg;
    s->seek(s->get_target() + value);
}

static void on_minus(char cmd, int value, void *arg)
{
    Stepper *s = (Stepper*) arg;
    s->seek(s->get_target() - value);
}

    /*
    *   command handler Actions
    */

static Action actions[] = {
    { 'G', on_g, & stepper, 0 },
    { 'S', on_s, & stepper, 0 },
    { 'R', on_r, & stepper, 0 },
    { 'Z', on_z, & stepper, 0 },
    { '+', on_plus, & stepper, 0 },
    { '-', on_minus, & stepper, 0 },
    { '\0', 0, 0, 0 },
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

    const int diff = strcmp(last, buff);
    const bool ready = buff[0] == 'R';

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

void setup () {
    Serial.begin(9600);

    pinMode(sensor_0, INPUT);
    pinMode(sensor_1, INPUT);

    // Add handlers to CLI
    for (Action *a = actions; a->cmd; a++)
    {
        cli.add_action(a);
    };
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
