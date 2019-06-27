
#include "motor.h"

#include <Arduino.h>

  /*
  *
  */

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

  /*
  *
  */

Stepper::Stepper(int cycle, int p1, int p2, int p3, int p4, int time)
: state(0), steps(cycle), count(0), target(0), rotate_to(-1), period(time)
{
    pins[0] = p1;
    pins[1] = p2;
    pins[2] = p3;
    pins[3] = p4;

    for (int i = 0; i < PINS; i++)
    {
        pinMode(pins[i], OUTPUT);
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

int Stepper::clip(int t)
{
    // clip to valid limits
    if (t < 0)
    {
        return 0;
    }

    if (t >= steps)
    {
        return steps - 1;
    }

    return t;
}

void Stepper::seek(int t)
{
    target = clip(t);
}

void Stepper::rotate(int t)
{
    while (t < 0)
    {
        t += steps;
    }

    t %= steps;

    // only set if we're aren't there already
    if (t != count)
    {
        rotate_to = t;
    }
}

int Stepper::get_target()
{
    return target;
}

bool Stepper::ready()
{
    return get_delta() == 0;
}

void Stepper::zero(int t)
{
    target = t;
    count = t;
}

void Stepper::set_steps(int s)
{
    steps = s;
}

int Stepper::get_steps()
{
    return steps;
}

int Stepper::get_delta()
{
    if (rotate_to == -1)
    {
        return target - count;
    }

    // which direction to move?
    int d = rotate_to - count;
    const int half = steps / 2;

    if (d == 0)
    {
        return 0;
    }

    if (d > 0)
    {
        // d is +ve
        if (d < half)
        {
            // move forward
            return d;
        }
        // move backwards
        return -d;
    }

    // d is -ve
    if (d >= -half)
    {
        // move backwards
        return d;
    }

    // move forward
    return -d;
}

void Stepper::poll()
{
    int delta = get_delta();

    if (delta == 0)
    {
        return;
    }

    step(delta > 0);

    if ((rotate_to != -1) && (rotate_to == count))
    {
        //  we've arrived
        rotate_to = -1;
        target = count;
    }

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

// FIN
