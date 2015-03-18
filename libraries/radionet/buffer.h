/*
 Arduino library to read data from an Elster A100C electricity meter.

 Copyright (C) 2012 Dave Berkeley projects@rotwang.co.uk

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

#define BUFF_SIZE 128

struct Buffer
{
  int data[BUFF_SIZE];
  int in;
  int out;
};

static void buff_init(struct Buffer* b)
{
  b->in = b->out = 0;
}

static int buff_full(struct Buffer* b)
{
  int next = b->in + 1;
  if (next >= BUFF_SIZE)
    next = 0;
  return next == b->out;
}

static int buff_add(struct Buffer* b, int d)
{
  int next = b->in + 1;
  if (next >= BUFF_SIZE)
    next = 0;
  if (next == b->out)
    return -1; // overfow error

  b->data[b->in] = d;
  b->in = next;  
  return 0;  
}

static int buff_get(struct Buffer* b, int* d)
{
  if (b->in == b->out)
    return -1;
  int next = b->out + 1;
  if (next >= BUFF_SIZE)
    next = 0;
  *d = b->data[b->out];
  b->out = next;
  return 0;  
}

// FIN
