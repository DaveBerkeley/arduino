
#include <stdlib.h>

#include <gtest/gtest.h>

#include "Arduino.h"
#include "mock.h"

#include "../cli.h"
#include "../motor.h"

typedef struct {
    char cmd;
    int value;
    void (*fn)(char cmd, int value, void *arg);
}   CliArg;

static void on_a(char cmd, int value, void *arg)
{
    ASSERT(arg);
    CliArg *cli_arg = (CliArg*) arg;

    cli_arg->cmd = cmd;
    cli_arg->value = value;
    cli_arg->fn = on_a;
}

static void on_b(char cmd, int value, void *arg)
{
    ASSERT(arg);
    CliArg *cli_arg = (CliArg*) arg;

    cli_arg->cmd = cmd;
    cli_arg->value = value;
    cli_arg->fn = on_b;
}

    /*
     *
     */

TEST(Cli, Cli)
{
    CLI cli;
    CliArg arg;

    Action a = { 'A', on_a, & arg, 0 };
    cli.add_action(& a);

    // check A
    memset(& arg, 0, sizeof(arg));
    cli.process("A1234\r\n");
    EXPECT_EQ(arg.cmd, 'A');
    EXPECT_EQ(arg.value, 1234);
    EXPECT_EQ(arg.fn, on_a);

    Action b = { 'B', on_b, & arg, 0 };
    cli.add_action(& b);

    // check B
    memset(& arg, 0, sizeof(arg));
    cli.process("B0\r\n");
    EXPECT_EQ(arg.cmd, 'B');
    EXPECT_EQ(arg.value, 0);
    EXPECT_EQ(arg.fn, on_b);

    // check A still works
    memset(& arg, 0, sizeof(arg));
    cli.process("A1234\r\n");
    EXPECT_EQ(arg.cmd, 'A');
    EXPECT_EQ(arg.value, 1234);
    EXPECT_EQ(arg.fn, on_a);

    // check some non commands
    memset(& arg, 0, sizeof(arg));
    cli.process("123\r\n");
    EXPECT_EQ(arg.cmd, '\0');
    EXPECT_EQ(arg.value, 0);
    EXPECT_EQ(arg.fn, (void*) 0);

    // check no command
    memset(& arg, 0, sizeof(arg));
    cli.process("\r\n");
    EXPECT_EQ(arg.cmd, '\0');
    EXPECT_EQ(arg.value, 0);
    EXPECT_EQ(arg.fn, (void*) 0);

    // check default 0
    memset(& arg, 0, sizeof(arg));
    cli.process("A\r\n");
    EXPECT_EQ(arg.cmd, 'A');
    EXPECT_EQ(arg.value, 0);
    EXPECT_EQ(arg.fn, on_a);

    // check <backspace> deletes (reset)
    // actually, any unknown cahr will reset the line
    memset(& arg, 0, sizeof(arg));
    cli.process("A\b\r\n");
    EXPECT_EQ(arg.cmd, '\0');
    EXPECT_EQ(arg.value, 0);
    EXPECT_EQ(arg.fn, (void*)0);
}

    /*
     *
     */

static void seek_test(Stepper *stepper, int seek)
{
    stepper->seek(seek);
    const int start = stepper->position();
    int step = 1;
    if (seek < start)
    {
        step = -1;
    }

    for (int i = start; i != seek; i += step)
    {
        EXPECT_EQ(stepper->position(), i);
        EXPECT_EQ(stepper->ready(), false);
        // step to the next position
        stepper->poll();
    }

    // should now be in position
    EXPECT_EQ(stepper->position(), seek);
    EXPECT_EQ(stepper->ready(), true);
}

    /*
     *
     */

TEST(Motor, Seek)
{
    mock_setup();

    int cycle = 5000;
    Stepper stepper(cycle, 1, 2, 3, 4);

    // seek test
    EXPECT_EQ(stepper.position(), 0);
    seek_test(& stepper, 100);
    EXPECT_EQ(stepper.get_target(), 100);
    seek_test(& stepper, 0);
    seek_test(& stepper, 4999);
    seek_test(& stepper, 50);
    seek_test(& stepper, 1050);
    EXPECT_EQ(stepper.get_target(), 1050);

    // zero test
    stepper.zero();
    EXPECT_EQ(stepper.position(), 0);
    seek_test(& stepper, 100);

    // seek should clip at limits (<0)
    seek_test(& stepper, 100);
    stepper.seek(-1);
    while (!stepper.ready())
    {
        stepper.poll();
    }
    EXPECT_EQ(stepper.position(), 0);

    // seek should clip at limits (>= cycle)
    stepper.seek(cycle + 100);
    while (!stepper.ready())
    {
        stepper.poll();
    }
    EXPECT_EQ(stepper.position(), cycle - 1);

    // seek should clip at limits (>= cycle)
    stepper.seek(cycle);
    while (!stepper.ready())
    {
        printf("%d\n", stepper.position());
        stepper.poll();
    }
    EXPECT_EQ(stepper.position(), cycle - 1);

    mock_teardown();
}

    /*
     *  Check the state of the pins
     *
     *  They should follow a strict pattern
     *
     *  1000
     *  1001
     *  0011
     *  0010
     *  0110
     *  0100
     *  1100
     *
     *  repeated
     */

TEST(Motor, IO)
{
    mock_setup();

    int cycle = 5000;
    Stepper stepper(cycle, 1, 2, 3, 4);

    int pins[4] = { 0, 0, 0, 0 };

    EXPECT_EQ(stepper.position(), 0);

    // starts with begining pin hi
    pins[0] = 1;
    EXPECT_TRUE(pins_match(4, 1, pins));
 
    // one pin changes state on each step
    seek_test(& stepper, 1);
    pins[3] = 1;
    EXPECT_TRUE(pins_match(4, 1, pins));

    seek_test(& stepper, 2);
    pins[0] = 0;
    EXPECT_TRUE(pins_match(4, 1, pins));

    seek_test(& stepper, 3);
    pins[2] = 1;
    EXPECT_TRUE(pins_match(4, 1, pins));

    seek_test(& stepper, 4);
    pins[3] = 0;
    EXPECT_TRUE(pins_match(4, 1, pins));

    seek_test(& stepper, 5);
    pins[1] = 1;
    EXPECT_TRUE(pins_match(4, 1, pins));

    seek_test(& stepper, 6);
    pins[2] = 0;
    EXPECT_TRUE(pins_match(4, 1, pins));

    seek_test(& stepper, 7);
    pins[0] = 1;
    EXPECT_TRUE(pins_match(4, 1, pins));

    // back to the start of the pin sequence
    seek_test(& stepper, 8);
    pins[1] = 0;
    EXPECT_TRUE(pins_match(4, 1, pins));

    mock_teardown();
}

//  FIN
