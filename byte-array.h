#ifndef BYTEARRAY_H
#define BYTEARRAY_H

#include "vector"

using namespace std;

class ByteArray
{
public:
  vector<byte> data;

  ByteArray();
  ~ByteArray();

  vector<byte> getData();

  void writeByte(byte b);

  void writeByte(int b);

  void writeUTFBytes(string s);

  void writeBytes(vector<byte> arr, int offset, int length);

  void writeBytes(byte arr[], int offset, int length);

  void writeBytes(vector<byte> bytes);
};

#endif