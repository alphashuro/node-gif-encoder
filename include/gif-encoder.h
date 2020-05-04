#ifndef GIFENCODER_H
#define GIFENCODER_H

#include <boost/optional.hpp>
#include <vector>
#include "map"
#include "byte-array.h"

using namespace std;

namespace gifencoder
{

class GIFEncoder
{
public:
  vector<char> image; // current frame
  void getImagePixels();

  int width, height;

  // transparent color if given
  boost::optional<int> transparent;

  // transparent index in color table
  unsigned char transIndex = 0;

  // -1 = no repeat, 0 = forever. anything else is repeat count
  int repeat = -1;

  // frame delay (hundredths)
  unsigned int delay = 0;

  vector<char> pixels;        // BGR int array from frame
  vector<char> indexedPixels; // converted frame indexed to palette
  int colorDepth = 8;         // number of bit planes
  vector<int> colorTab;       // RGB palette
  map<int, bool> usedEntry;   // active palette entries
  int palSize = 7;            // color table size (bits-1)
  int dispose = -1;           // disposal code (-1 = use default)
  bool firstFrame = true;
  int sample = 10; // default sample interval for quantizer

  bool started = false; // started encoding

  // vector<Functions> readStreams = [];

  ByteArray out;

  explicit GIFEncoder(int w = 0, int h = 0);
  ~GIFEncoder();

  void start();
  void finish();
  void setRepeat(int r);
  /*
    Sets quality of color quantization (conversion of images to the maximum 256
    colors allowed by the GIF specification). Lower values (minimum = 1)
    produce better colors, but slow processing significantly. 10 is the
    default, and produces good color mapping at reasonable speeds. Values
    greater than 20 do not yield significant improvements in speed.
  */
  void setQuality(int q);
  /*
    Sets frame rate in frames per second.
  */
  void setFrameRate(int fps);
  void addFrame(vector<char> &frame);
  void writePixels();
  void analyzePixels();
  int findClosest(int c);
  void writeShort(int pValue);
  void writeLSD();
  void writePalette();
  void writeNetscapeExt();
  void writeGraphicCtrlExt();
  void writeImageDesc();
};

} // namespace gifencoder

#endif