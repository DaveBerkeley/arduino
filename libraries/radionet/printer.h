
#if ! defined(_PRINTER_H_)
#define _PRINTER_H_

#include <stdarg.h>

class Output
{
public:
    virtual void put_char(char c) = 0;
    virtual void put_str(char const* s)
    {
        while (*s) put_char(*s++);
    }
};

void voprintf(Output& out, char const* fmt, va_list& va);
void oprintf(Output& out, char const* fmt, ...);

#endif // _PRINTER_H_

//  FIN
