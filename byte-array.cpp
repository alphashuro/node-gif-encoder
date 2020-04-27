#include "byte-array.h"

vector<int> ByteArray::getData()
{
  return data;
};

void ByteArray::writeByte(int b)
{
  data.push_back(b);
};

void ByteArray::writeUTFBytes(string s)
{
  for (char c : s)
    writeByte(int(c));
}

void ByteArray::writeBytes(vector<int> arr, int offset, int length)
{
  data.insert(data.begin() + offset, arr.begin(), arr.begin() + length);
}

void ByteArray::writeBytes(int arr[], int offset, int length)
{
  data.insert(data.begin() + offset, arr, arr + length);
}

void ByteArray::writeBytes(vector<int> bytes)
{
  data.insert(data.begin(), bytes.begin(), bytes.end());
}
