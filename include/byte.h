#ifndef BYTE_H
#define BYTE_H

namespace std
{
enum class byte : unsigned char
{
};

byte operator&(byte l, int r);

byte operator-(byte l, byte r);

byte operator*(byte l, byte r);

byte operator+(byte l, byte r);

bool operator<(byte l, byte r);

byte operator>>(byte l, int r);

} // namespace std

#endif