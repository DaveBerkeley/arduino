
#include "cli.h"

  /*
  *
  */

void CLI::reset()
{
    command = 0;
    value = 0;
    get_value = 0;
}

void CLI::run()
{
    for (Action *action = actions; actions; action = action->next)
    {
        if (action->cmd == command)
        {
            action->fn(command, value, action->arg);
            reset();
            return;
        }
    }
}

CLI::CLI()
{
    reset();
}

void CLI::add_action(Action *action)
{
    action->next = actions;
    actions = action;
}

void CLI::process(char c)
{
    // process any completed command line
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

    // handle numeric values
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

    // check if the char is a valid command
    for (Action *action = actions; action; action = action->next)
    {
        if (action->cmd == c)
        {
            command = c;
            get_value = true;
            value = 0;
            return;
        }
    }

    // the char doesn't match any known options
    reset();
}

// FIN
