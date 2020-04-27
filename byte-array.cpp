#include "byte-array.h"

class ByteArray
{
public:
  vector<byte> data = {};

  ByteArray(){};
  ~ByteArray(){};

  vector<byte> getData()
  {
    return data;
  };

  void writeByte(byte b)
  {
    data.push_back(b);
  };

  void writeByte(int b)
  {
    data.push_back(byte(b));
  };

  void writeUTFBytes(string s)
  {
    for (char c : s)
      writeByte(byte(c));
  }

  void writeBytes(vector<byte> arr, int offset, int length)
  {
    data.insert(data.begin() + offset, arr.begin(), arr.begin() + length);
  }

  void writeBytes(byte arr[], int offset, int length)
  {
    data.insert(data.begin() + offset, arr, arr + length);
  }

  void writeBytes(vector<byte> bytes)
  {
    data.insert(data.begin(), bytes.begin(), bytes.end());
  }
};
