#ifndef BYTEARRAY_H
#define BYTEARRAY_H

#include "vector"
#include "string"

using namespace std;

namespace gifencoder
{
class ByteArray
{
public:
  vector<unsigned char> data;

  ByteArray();
  ~ByteArray();

  vector<unsigned char> &getData();

  void writeByte(int b);

  void writeByte(char b);

  void writeUTFBytes(const string s);

  void writeBytes(const vector<int> &arr, int offset, int length);

  void writeBytes(const int arr[], int offset, int length);

  void writeBytes(const vector<int> &bytes);
};

} // namespace gifencoder

#endif