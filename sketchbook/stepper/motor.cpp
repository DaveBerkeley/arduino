
#include "motor.h"

#include <Arduino.h>

  /*
  *
  */

Stepper::Stepper(int cycle, int p1, int p2, int p3, int p4, int time)
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

void Stepper::set_state(int s)
{
    const int* states = cycle[s];
    for (int i = 0; i < PINS; i++)
    {
        digitalWrite(pins[i], states[i]);
    }
    state = s;
}

void Stepper::step(bool up)
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

int Stepper::position()
{
    return count;
}

void Stepper::seek(int t)
{
    target = t;
}

int Stepper::get_target()
{
    return target;
}

bool Stepper::ready()
{
    return target == count;
}

void Stepper::zero()
{
    target = 0;
    count = 0;
}

void Stepper::set_steps(int s)
{
    steps = s;
}

void Stepper::poll()
{
    const int delta = count - target;

    if (delta == 0)
        return;

    step(delta < 0);

    const int move = abs(delta);

    // Acceleration / deceleration
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

// FIN
