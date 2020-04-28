#ifndef BYTE_H
#define BYTE_H

#include "byte.h"

namespace std
{
byte operator&(byte l, int r)
{
  return byte(int(l) & r);
};

byte operator-(byte l, byte r)
{
  return byte(int(l) - int(r));
};

byte operator*(byte l, byte r)
{
  return byte(int(l) * int(r));
};

byte operator+(byte l, byte r)
{
  return byte(int(l) + int(r));
};

bool operator<(byte l, byte r)
{
  return int(l) < int(r);
};

byte operator>>(byte l, int r)
{
  return byte(int(l) >> r);
};

} // namespace std

#endif