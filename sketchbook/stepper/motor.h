
  /*
  *
  */

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
    int period;

public:
    Stepper(int cycle, int p1, int p2, int p3, int p4, int time=1000);

    void set_state(int s);
    void step(bool up);
    int position();
    void seek(int t);
    int get_target();
    bool ready();
    void zero();
    void set_steps(int s);
    void poll();
};

// FIN