/*
 Copyright (C) 2015 Dave Berkeley projects2@rotwang.co.uk

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 USA
*/

#include <stdint.h>
#include <stdbool.h>

#include "pinio.h"

    /*
     *  Pin IO functions.
     */

void pin_mode(const PinIo* pin, bool output)
{
    pin_change(pin->ddr, pin->mask, output);
}

void pin_set(const PinIo* pin, bool state)
{
    pin_change(pin->data, pin->mask, state);
}

bool pin_get(const PinIo* pin)
{
    return (pin->mask & *(pin->pin)) ? true : false;
}

//  FIN
