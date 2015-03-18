
#include <stdint.h>
#include <string.h>

#include "printer.h"

//  printer.cpp
//
//  http://www.cplusplus.com/reference/cstdio/printf/
//

typedef struct {
    uint8_t flags; // "+- #0"
    uint8_t width; // n or *
    uint8_t precision; // n or *
    uint8_t length[2]; // hl...
}   format;

class Printer
{
private:
    Output& m_out;
public:
    Printer(Output& out) : m_out(out) { }

    //  delegate put fns to Output

    void put_char(char c)
    {
        m_out.put_char(c);
    }

    void put_str(char const* s)
    {
        m_out.put_str(s);
    }

private:
    static inline char hex(uint8_t n)
    {
        if (n < 10)
            return n + '0';
        return n - 10 + 'A';
    }

    void print_num(int32_t n, uint8_t lead, uint8_t digits, uint8_t base)
    {
        uint32_t num = n;
        bool minus = false;

        if (base == 10)
        {
            if (n < 0)
            {
                num = -n;
                if (digits)
                    digits -= 1; //  for the '-' sign
                minus = true;
            }
        }
        else
        {
            lead = '0';
        }

        char buff[12]; // big enough for 1<<32
        const char* end = & buff[sizeof(buff)];

        char* d = buff;

        while (true)
        {
            *d++ = hex(num % base);
            num /= base;

            if ((d == end) || (num == 0))
                break;
        }

        if (lead != '0')
            lead = ' ';

        if (minus && (lead == '0'))
            put_char('-');

        if (digits)
        {
            const uint8_t used = d - buff;

            for (uint8_t i = digits - used; i > 0; i--)
            {
                put_char(lead);
            }
        }

        if (minus && (lead == ' '))
            put_char('-');

        while (--d >= buff)
            put_char(*d);
    }

    void print_spaces(int8_t spaces)
    {
        for (; spaces > 0; spaces--)
            put_char(' ');
    }

public:
    void print(char type, const format& f, va_list& va)
    {
        switch (type)
        {
            case 'd' :
            case 'i' :
            {
                const int32_t d = va_arg(va, int);
                print_num(d, f.flags, f.width, 10);
                break;
            }
            case 'x' :
            case 'X' :
            {
                if (f.flags == '#')
                {
                    put_str("0x");
                }
                const int32_t d = va_arg(va, int);
                print_num(d, f.flags, f.width, 16);
                break;
            }
            case 's' :
            {
                //  print string
                char const* s = va_arg(va, char*);

                if ((f.flags != '-') && f.width)
                {
                    print_spaces(f.width - strlen(s));
                }

                if (f.width)
                {
                    for (uint8_t i = 0; *s && (i < f.width); i++)
                        put_char(*s++);
                }
                else
                {
                    put_str(s);
                }

                if ((f.flags == '-') && f.width)
                {
                    print_spaces(f.width - strlen(s));
                }

                break;
            }
            case 'p' :
            {
                char* p = va_arg(va, char*);
                put_str("0x");
                print_num((uint32_t) p, '0', sizeof(p)*2, 16);
                break;
            }
            default:
                //printf("Bad type '%c'!\n", type);
                ;
        }
    }
};

    /*
     *
     */

static void get_num(uint8_t* n, char const*& fmt)
{
    *n = 0;
    while (('0' <= *fmt) && ('9' >= *fmt))
    {
        *n *= 10;
        *n += (*fmt++) - '0';
    }
}

static void parse_fmt(format& f, va_list& va, char const*& fmt)
{
    static char const* digits = "0123456789";

    //  flags
    if (strchr("+-#0 ", *fmt))
    {
        f.flags = *fmt++;
    }

    //  width
    if (strchr(digits, *fmt))
    {
        get_num(& f.width, fmt);
    }
    else if (*fmt == '*')
    {
        f.width = va_arg(va, int);
        fmt += 1;
    }

    //  precision
    if (*fmt == '.')
    {
        get_num(& f.precision, ++fmt);
    }
    else if (*fmt == '*')
    {
        f.precision = va_arg(va, int);
        fmt += 1;
    }

    //  length
    for (uint8_t i = 0; i < sizeof(f.length); ++i)
    {
        if (strchr("hl", *fmt))
        {
            f.length[i] = *+fmt++;
        }
    }
}

static void voprintf(Printer& out, char const* fmt, va_list& va)
{
    while (*fmt)
    {
        if (*fmt != '%')
        {
            out.put_char(*fmt++);
            continue;
        }

        // read all the fmt chars up to the specifier
        format f = { 0 };
        parse_fmt(f, va, ++fmt);

        out.print(*fmt++, f, va);
    }
}

void voprintf(Output& out, char const* fmt, va_list& va)
{
    Printer printer(out);
    voprintf(printer, fmt, va);
}

void oprintf(Output& out, char const* fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    voprintf(out, fmt, va);
    va_end(va);
}

//  test.cpp

#if defined(TEST)

#include <stdio.h>

class StdOut : public Output
{
    virtual void put_char(char c) { ::putchar(c); }
};

int main()
{
    StdOut out;

    oprintf(out, "Hello world!\n");
    oprintf(out, "'%08d'\n", 1234);
    oprintf(out, "'%08d'\n", -1234);
    oprintf(out, "'%08x'\n", 0x89A1FACE);
    oprintf(out, "'%#8x'\n", 0xFACE);
    oprintf(out, "'%s'\n", "hello world!");
    oprintf(out, "'%d'\n", -1234);
    oprintf(out, "'%0*d'\n", 10, -1234);
    oprintf(out, "'% *d'\n", 20, -1234);
    oprintf(out, "'%-*s'\n", 10, "hello");
    oprintf(out, "'%-10s'\n", "hello");
    oprintf(out, "%d\n", 0);
    oprintf(out, "%p\n", & out);
    return 0;
}

#endif

//  FIN
