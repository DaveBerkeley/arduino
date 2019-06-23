
  /*
  *
  */

typedef struct Action
{
    const char cmd;
    void (*fn)(char cmd, int value, void *arg);
    void *arg;
    struct Action *next;
}   Action;

class CLI
{
    char command;
    int value;
    bool get_value;
    Action *actions;

    void reset();
    void run();

public:
    CLI();

    void add_action(Action *action);
    void process(char c);
};

// FIN
