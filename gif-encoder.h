#ifndef GIFENCODER_H
#define GIFENCODER_H

#include <node.h>
#include <node_object_wrap.h>
#include <boost/optional.hpp>
#include <vector>
// #include "int.h"
#include "lzw-encoder.h"
#include "typed-neu-quant.h"
#include "cmath"
#include "node_buffer.h"
#include "byte-array.h"

using namespace std;

namespace gifencoder
{

class GIFEncoder : public node::ObjectWrap
{
public:
  static void Init(v8::Local<v8::Object> exports, v8::Local<v8::Object> module);

  vector<int> image; // current frame
  void getImagePixels();

private:
  int width, height;

  // transparent color if given
  boost::optional<int> transparent;

  // transparent index in color table
  unsigned int transIndex = 0;

  // -1 = no repeat, 0 = forever. anything else is repeat count
  int repeat = -1;

  // frame delay (hundredths)
  unsigned int delay = 0;

  vector<int> pixels;        // BGR int array from frame
  vector<int> indexedPixels; // converted frame indexed to palette
  int colorDepth = 8;        // number of bit planes
  vector<int> colorTab;      // RGB palette
  vector<bool> usedEntry;    // active palette entries
  int palSize = 7;           // color table size (bits-1)
  int dispose = -1;          // disposal code (-1 = use default)
  bool firstFrame = true;
  int sample = 10; // default sample interval for quantizer

  bool started = false; // started encoding

  // vector<Functions> readStreams = [];

  ByteArray out = ByteArray();

  explicit GIFEncoder(int w = 0, int h = 0);
  ~GIFEncoder();

  static void New(const v8::FunctionCallbackInfo<v8::Value> &args);
  static void Start(const v8::FunctionCallbackInfo<v8::Value> &args);
  static void SetRepeat(const v8::FunctionCallbackInfo<v8::Value> &args);
  /*
        Sets quality of color quantization (conversion of images to the maximum 256
        colors allowed by the GIF specification). Lower values (minimum = 1)
        produce better colors, but slow processing significantly. 10 is the
        default, and produces good color mapping at reasonable speeds. Values
        greater than 20 do not yield significant improvements in speed.
      */
  static void SetQuality(const v8::FunctionCallbackInfo<v8::Value> &args);
  /*
        Sets frame rate in frames per second.
      */
  static void SetFrameRate(const v8::FunctionCallbackInfo<v8::Value> &args);
  static void AddFrame(const v8::FunctionCallbackInfo<v8::Value> &args);

  void writePixels();
  void analyzePixels();
  int findClosest(int c);
  // void writeShort(int pValue);
  void writeShort(int pValue);
  void writeLSD();
  void writePalette();
  void writeNetscapeExt();
  void writeGraphicCtrlExt();
  void writeImageDesc();
};

} // namespace gifencoder

#endif