
#include <string.h>

#include "cli.h"

  /*
  *
  */

void CLI::reset()
{
    memset(command, 0, sizeof(command));
    match = 0;
    memset(values, 0, sizeof(values));
    idx = 0;
    num_valid = false;
    negative = false;
}

void CLI::run()
{
    if (match)
    {
        match->fn(match, idx, values);
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

bool CLI::match_action(Action **a)
{
    for (Action *action = actions; action; action = action->next)
    {
        if (!strncmp(action->cmd, command, idx))
        {
            if (idx == (int) strlen(action->cmd))
            {
                *a = action;
            }
            return true;
        }
    }
    return false;
}

void CLI::process(char c)
{
    // check if the char is a valid command
    if (!match)
    {
        // add char to cmd buffer
        command[idx++] = c;

        if (idx >= MAX_CMD)
        {
            reset();
            return;
        }

        Action *a = 0;
        if (match_action(& a))
        {
            if (a)
            {
                idx = 0;
                match = a;
            }
            return;
        }
        reset();
        return;
    }

    // process any completed line
    if ((c == '\r') || (c == '\n'))
    {
        if (num_valid)
        {
            apply_sign();
            // make sure that the trailing value gets counted
            idx += 1;
        }
        run();
        reset();
        return;
    }

    // handle numeric values
    if ((c >= '0') && (c <= '9'))
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
        if (!num_valid)
        {
            negative = c == '-';
        }
        else
        {
            reset();
        }
        return;
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
