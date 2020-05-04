#include "byte-array.h"

namespace gifencoder
{

ByteArray::ByteArray(){};
ByteArray::~ByteArray(){};

vector<unsigned char>& ByteArray::getData()
{
  return data;
};

void ByteArray::writeByte(char c)
{
  data.push_back(c);
};

void ByteArray::writeByte(int b)
{
  data.push_back(b);
};

void ByteArray::writeUTFBytes(const string s)
{
  for (char c : s)
    writeByte(c);
}

void ByteArray::writeBytes(const vector<int> &arr, int offset, int length)
{
  data.insert(data.end(), arr.begin() + offset, arr.begin() + offset + length);
}

void ByteArray::writeBytes(const int arr[], int offset, int length)
{
  data.insert(data.end(), arr + offset, arr + offset + length);
}

void ByteArray::writeBytes(const vector<int> &bytes)
{
  data.insert(data.end(), bytes.begin(), bytes.end());
}

// template <size_t T>
// void ByteArray::writeBytes(const array<int, T> &bytes) {
//   data.insert(data.end(), bytes.begin(), bytes.end());
// }

} // namespace gifencoder