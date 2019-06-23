
#include <stdlib.h>

#include <gtest/gtest.h>

#include <Arduino.h>

#include "../cli.h"

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
}

//  FIN
