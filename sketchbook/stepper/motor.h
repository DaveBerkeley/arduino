
  /*
  *
  */

#include <stdint.h>

class Stepper
{
    const static int PINS = 4;
    int pins[PINS];

    const static int STATES = 8;
    const static int cycle[STATES][PINS];

    int state;
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
    Stepper(int cycle, int p1, int p2, int p3, int p4, uint32_t time=1000);

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
};

// FIN
