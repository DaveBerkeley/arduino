
    /*
     *
     */

#include <stdint.h>

    /*
     *  Abstract Stepper Interface
     */

class MotorIo
{
public:
    virtual void step(bool up) = 0;
    virtual void power(bool on) = 0;
};

    /*
     *  Four wire uni-polar stepper
     */

class MotorIo_4 : public MotorIo
{
    const static int PINS = 4;
    int pins[PINS];

    const static int STATES = 8;
    const static int cycle[STATES][PINS];

    int state;

    void set_state(int s);

public:
    MotorIo_4(int p1, int p2, int p3, int p4);

    virtual void step(bool up);
    virtual void power(bool on);
};

   /*
    *
    */

class Stepper
{
    MotorIo *io;
    int steps;
    int count;
    int target;
    int rotate_to;
    uint32_t period;

    enum Accel { ACCEL, DECEL, NONE };
    enum Accel accel;
    int reference;

    void set_state(int s);
    void step(bool up);
    int get_delta();
    void set_accel();

public:
    Stepper(int cycle, MotorIo* io, uint32_t time=1000);

    int position();
    void seek(int t);
    void rotate(int t);
    int get_target();
    int get_steps();
    int clip(int t);
    bool ready();
    void zero(int t=0);
    void set_steps(int s);
    void poll();
    void power(bool on);
};

// FIN
