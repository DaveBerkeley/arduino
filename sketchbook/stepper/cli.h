
  /*
  *
  */

typedef struct Action
{
    const char cmd;
    void (*fn)(char cmd, int argc, int *argv, void *arg);
    void *arg;
    struct Action *next;
}   Action;

class CLI
{
public:
    static const int MAX_VALUES = 4;
private:
    char command;
    int idx;
    bool num_valid;
    Action *actions;
    int values[MAX_VALUES];

    const char *delim;

    void reset();
    void run();

public:
    CLI(const char *delimit=",");

    void add_action(Action *action);
    void process(char c);
    void process(const char *line);
};

// FIN
