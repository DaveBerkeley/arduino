
#include <string.h>

#include "cli.h"

  /*
  *
  */

void CLI::reset()
{
    command = 0;
    for (int i = 0; i < MAX_VALUES; i++)
    {
        values[i] = 0;
    }
    idx = 0;
    num_valid = false;
    negative = false;
}

void CLI::run()
{
    for (Action *action = actions; actions; action = action->next)
    {
        if (action->cmd == command)
        {
            action->fn(command, idx, values, action->arg);
            return;
        }
    }
}

CLI::CLI(const char *delimit)
:   actions(0),
    delim(delimit)
{
    reset();
}

void CLI::add_action(Action *action)
{
    action->next = actions;
    actions = action;
}

void CLI::apply_sign()
{
    if (negative)
    {
        values[idx] = -values[idx];
        negative = false;
    }
}

void CLI::process(char c)
{
    // process any completed line
    if ((c == '\r') || (c == '\n'))
    {
        if (command)
        {
            if (num_valid)
            {
                apply_sign();
                // make sure that the trailing value gets counted
                idx += 1;
            }
            run();
        }
        reset();
        return;
    }

    // handle numeric values
    if (command && ((c >= '0') && (c <= '9')))
    {
        // numeric
        values[idx] *= 10;
        values[idx] += c - '0';
        num_valid = true;
        return;
    }

    // handle '-'/'+' for sign
    if ((c == '-') || (c == '+'))
    {
        if (command && !num_valid)
        {
            negative = c == '-';
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
            return;
        }
    }

    // check for delimiter
    if (strchr(delim, c))
    {
        if (num_valid)
        {
            apply_sign();
            idx += 1;
        }
        num_valid = false;
        if (idx >= MAX_VALUES)
        {
            reset();
        }
        return;
    }

    // the char doesn't match any known options
    reset();
}

void CLI::process(const char *line)
{
    while (*line)
    {
        process(*line++);
    }
}

// FIN
