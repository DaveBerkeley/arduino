
  /*
  *
  */

typedef struct Action
{
    const char *cmd;
    void (*fn)(Action *action, int argc, int *argv);
    void *arg;
    struct Action *next;
}   Action;

class CLI
{
public:
    static const int MAX_VALUES = 10;
    static const int MAX_CMD = 6;
private:
    char command[MAX_CMD];
    int cmd_idx;
    int idx;
    bool num_valid;
    bool negative;
    Action *actions;
    int values[MAX_VALUES];
    Action *match;

    const char *delim;

    void reset();
    void run();
    void apply_sign();
    bool match_action(Action **a);

public:
    CLI(const char *delimit=",");

    void add_action(Action *action);
    void process(char c);
    void process(const char *line);
};

// FIN
