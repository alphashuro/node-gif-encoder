#ifndef BYTEARRAY_H
#define BYTEARRAY_H

#include "vector"
#include "string"

using namespace std;

class ByteArray
{
public:
  vector<int> data;

  ByteArray();
  ~ByteArray();

  vector<int> getData();

  // void writeByte(int b);

  void writeByte(int b);

  void writeUTFBytes(string s);

  void writeBytes(vector<int> arr, int offset, int length);

  void writeBytes(int arr[], int offset, int length);

  void writeBytes(vector<int> bytes);
};

#endif