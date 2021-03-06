#include "gif-encoder.h"
#include "string"
#include "typed-neu-quant.h"
#include "lzw-encoder.h"
#include "cmath"
#include <chrono>
#include "iostream"

namespace gifencoder
{

GIFEncoder::GIFEncoder(int w, int h) : 
  width(~~w), 
  height(~~h), 
  pixels(new char[(w*h)*3]),
  pixLen((w * h) * 3),
  indexedPixels(new char[w * h])
{
};

GIFEncoder::~GIFEncoder(){
  // delete[] pixels;
  // delete[] indexedPixels;
};

void GIFEncoder::start()
{
  out.writeUTFBytes("GIF89a");
  started = true;
}

void GIFEncoder::finish()
{
  out.writeByte(0x3b);
}

void GIFEncoder::setRepeat(int r = 0)
{
  repeat = r;
}

void GIFEncoder::setQuality(int q)
{
  if (q < 1)
    q = 1;

  sample = q;
}

void GIFEncoder::setFrameRate(int fps)
{
  delay = round(100 / fps);
}

void GIFEncoder::addFrame(char* frame)
{
  image = frame;

  auto t1 = chrono::high_resolution_clock::now();
  getImagePixels(); // convert to correct format if necessary
  auto t2 = chrono::high_resolution_clock::now();

  // cout << "getImagePixels: " << chrono::duration_cast<chrono::microseconds>(t2 - t1).count() << endl;

  t1 = chrono::high_resolution_clock::now();
  analyzePixels(); // build color table & map pixels
  t2 = chrono::high_resolution_clock::now();

  // cout << "analyzePixels: " << chrono::duration_cast<chrono::microseconds>(t2 - t1).count() << endl;

  if (firstFrame)
  {
    t1 = chrono::high_resolution_clock::now();
    writeLSD(); // logical screen descriptior
    t2 = chrono::high_resolution_clock::now();

    // cout << "writeLSD: " << chrono::duration_cast<chrono::microseconds>(t2 - t1).count() << endl;

    t1 = chrono::high_resolution_clock::now();
    writePalette(); // global color table
    t2 = chrono::high_resolution_clock::now();
    // cout << "writePalette: " << chrono::duration_cast<chrono::microseconds>(t2 - t1).count() << endl;
    if (repeat >= 0)
    {
      t1 = chrono::high_resolution_clock::now();
      // use NS app extension to indicate reps
      writeNetscapeExt();
      t2 = chrono::high_resolution_clock::now();

      // cout << "writeNetscapeExt: " << chrono::duration_cast<chrono::microseconds>(t2 - t1).count() << endl;
    }
  }

  t1 = chrono::high_resolution_clock::now();
  writeGraphicCtrlExt(); // write graphic control extension
  t2 = chrono::high_resolution_clock::now();

  // cout << "writeGraphicCtrlExt: " << chrono::duration_cast<chrono::microseconds>(t2 - t1).count() << endl;

  t1 = chrono::high_resolution_clock::now();
  writeImageDesc(); // image descriptor
  t2 = chrono::high_resolution_clock::now();

  // cout << "writeImageDesc: " << chrono::duration_cast<chrono::microseconds>(t2 - t1).count() << endl;

  if (!firstFrame)
  {

    t1 = chrono::high_resolution_clock::now();
    writePalette(); // local color table
    t2 = chrono::high_resolution_clock::now();

    // cout << "writePalette: " << chrono::duration_cast<chrono::microseconds>(t2 - t1).count() << endl;
  }
  t1 = chrono::high_resolution_clock::now();
  writePixels(); // encode and write pixel data
  t2 = chrono::high_resolution_clock::now();

  //******** cout << "writePixels: " << chrono::duration_cast<chrono::microseconds>(t2 - t1).count() << endl;

  firstFrame = false;
}

void GIFEncoder::getImagePixels()
{
  int k = 0;
  for (int i = 0; i < height; i++)
  {
    for (int j = 0; j < width; j++)
    {
      int b = (i * width * 4) + j * 4;

      pixels[k++] = image[b];
      pixels[k++] = image[b + 1];
      pixels[k++] = image[b + 2];
    }
  }
}

void GIFEncoder::writePixels()
{
  LZWEncoder enc = LZWEncoder(width, height, indexedPixels, colorDepth);

  enc.encode(out);
}

void GIFEncoder::analyzePixels()
{
  int len = pixLen;
  int nPix = len / 3;
  TypedNeuQuant imgq(pixels, sample, pixLen);

  auto t1 = chrono::high_resolution_clock::now();
  imgq.buildColormap(); // create reduced palette
  auto t2 = chrono::high_resolution_clock::now();

  // cout << "buildColormap: " << chrono::duration_cast<chrono::microseconds>(t2 - t1).count() << endl;

  t1 = chrono::high_resolution_clock::now();
  imgq.getColormap(colorTab);
  t2 = chrono::high_resolution_clock::now();

  // cout << "getColormap: " << chrono::duration_cast<chrono::microseconds>(t2 - t1).count() << endl;

  t1 = chrono::high_resolution_clock::now();
  // map image pixels to new palette
  int k = 0;
  for (int j = 0; j < nPix; j++)
  {
    int index = imgq.lookupRGB(
        pixels[k++] & 0xff,
        pixels[k++] & 0xff,
        pixels[k++] & 0xff);

    usedEntry[index] = true;
    indexedPixels[j] = index;
  }
  t2 = chrono::high_resolution_clock::now();
  // ********* cout << "lookupRGB: " << chrono::duration_cast<chrono::microseconds>(t2 - t1).count() << endl;

  colorDepth = 8;
  palSize = 7;

  // get closest match to transparent color if specified
  if (transparent.has_value())
  {
    t1 = chrono::high_resolution_clock::now();
    transIndex = findClosest(transparent.value());
    t2 = chrono::high_resolution_clock::now();
    cout << "findClosest: " << chrono::duration_cast<chrono::microseconds>(t2 - t1).count() << endl;

    t1 = chrono::high_resolution_clock::now();
    // ensure that pixels with full transparency in the RGBA image are using the selected transparent color index in the indexed image.
    for (int pixelIndex = 0; pixelIndex < nPix; pixelIndex++)
    {
      if (image[pixelIndex * 4 + 3] == char(0))
      {
        indexedPixels[pixelIndex] = transIndex;
      }
    }
    t2 = chrono::high_resolution_clock::now();
    cout << "ensure transparent: " << chrono::duration_cast<chrono::microseconds>(t2 - t1).count() << endl;
  }
}

/*
  Returns index of palette color closest to c
*/
int GIFEncoder::findClosest(int c)
{
  if (colorTabLen == 0)
    return -1;

  int r = (c & 0xFF0000) >> 16;
  int g = (c & 0x00FF00) >> 8;
  int b = (c & 0x0000FF);
  int minpos = 0;
  int dmin = int(256 * 256 * 256);
  int len = colorTabLen;

  for (int i = 0; i < len;)
  {
    int index = i / 3;
    int dr = r - (colorTab[i++] & 0xff);
    int dg = g - (colorTab[i++] & 0xff);
    int db = b - (colorTab[i++] & 0xff);
    int d = dr * dr + dg * dg + db * db;
    if (usedEntry[index] && (d < dmin))
    {
      dmin = d;
      minpos = index;
    }
  }

  return minpos;
};

/*
  Writes Logical Screen Descriptor
*/
void GIFEncoder::writeLSD()
{
  // logical screen size
  writeShort(width);
  writeShort(height);

  // packed fields
  out.writeByte(
      0x80 |  // 1 : global color table flag = 1 (gct used)
      0x70 |  // 2-4 : color resolution = 7
      0x00 |  // 5 : gct sort flag = 0
      palSize // 6-8 : gct size
  );

  out.writeByte(0); // background color index
  out.writeByte(0); // pixel aspect ratio - assume 1:1
};

void GIFEncoder::writeShort(int pValue)
{
  out.writeByte(pValue & 0xFF);
  out.writeByte((pValue >> 8) & 0xFF);
};

void GIFEncoder::writePalette()
{
  out.data.insert(out.data.end(), colorTab.begin(), colorTab.end());
  int n = (3 * 256) - colorTabLen;
  for (int i = 0; i < n; i++)
    out.writeByte(0);
}

/*
  Writes Netscape application extension to define repeat count.
*/
void GIFEncoder::writeNetscapeExt()
{
  out.writeByte(0x21);              // extension introducer
  out.writeByte(0xff);              // app extension label
  out.writeByte(11);                // block size
  out.writeUTFBytes("NETSCAPE2.0"); // app id + auth code
  out.writeByte(3);                 // sub-block size
  out.writeByte(1);                 // loop sub-block id
  writeShort(repeat);               // loop count (extra iterations, 0=repeat forever)
  out.writeByte(0);                 // block terminator
};

/*
  Writes Graphic Control Extension
*/
void GIFEncoder::writeGraphicCtrlExt()
{
  out.writeByte(0x21); // extension introducer
  out.writeByte(0xf9); // GCE label
  out.writeByte(4);    // data block size

  int transp, disp;
  if (!transparent.has_value())
  {
    transp = 0;
    disp = 0; // dispose = no action
  }
  else
  {
    transp = 1;
    disp = 2; // force clear if using transparent color
  }

  if (dispose >= 0)
  {
    disp = dispose & 7; // user override
  }
  disp <<= 2;

  // packed fields
  out.writeByte(
      0 |    // 1:3 reserved
      disp | // 4:6 disposal
      0 |    // 7 user input - 0 = none
      transp // 8 transparency flag
  );

  writeShort(int(delay));    // delay x 1/100 sec
  out.writeByte(transIndex); // transparent color index
  out.writeByte(0);          // block terminator
};

/*
  Writes Image Descriptor
*/
void GIFEncoder::writeImageDesc()
{
  out.writeByte(0x2c); // image separator
  writeShort(0);       // image position x,y = 0,0
  writeShort(0);
  writeShort(width); // image size
  writeShort(height);

  // packed fields
  if (firstFrame)
  {
    // no LCT - GCT is used for first (or only) frame
    out.writeByte(0);
  }
  else
  {
    // specify normal LCT
    out.writeByte(
        0x80 |  // 1 local color table 1=yes
        0 |     // 2 interlace - 0=no
        0 |     // 3 sorted - 0=no
        0 |     // 4-5 reserved
        palSize // 6-8 size of color table
    );
  }
}

} // namespace gifencoder
