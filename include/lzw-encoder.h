#ifndef LZWENCODER_H
#define LZWENCODER_H

/*
  LZWEncoder.js

  Authors
  Kevin Weiner (original Java version - kweiner@fmsware.com)
  Thibault Imbert (AS3 version - bytearray.org)
  Johan Nordberg (JS version - code@johan-nordberg.com)

  Acknowledgements
  GIFCOMPR.C - GIF Image compression routines
  Lempel-Ziv compression based on 'compress'. GIF modifications by
  David Rowley (mgardi@watdcsu.waterloo.edu)
  GIF Image compression - modified 'compress'
  Based on: compress.c - File compression ala IEEE Computer, June 1984.
  By Authors: Spencer W. Thomas (decvax!harpo!utah-cs!utah-gr!thomas)
  Jim McKie (decvax!mcvax!jim)
  Steve Davies (decvax!vax135!petsd!peora!srd)
  Ken Turkowski (decvax!decwrl!turtlevax!ken)
  James A. Woods (decvax!ihnp4!ames!jaw)
  Joe Orost (decvax!vax135!petsd!joe)
*/

#include "gif-encoder.h"

using namespace std;

namespace gifencoder
{
const int BITS = 12;
const int HSIZE = 5003; // 80% occupancy

class LZWEncoder
{
public:
  int width, height;
  char* pixels;
  int initCodeSize;
  int accum[256];
  int htab[HSIZE];
  int codetab[HSIZE];
  int cur_accum, cur_bits = 0;
  int a_count;
  int free_ent = 0; // first unused entry
  int maxcode;
  // block compression parameters -- after all codes are used up,
  // and compression rate changes, start over.
  bool clear_flg = false;
  // Algorithm: use open addressing double hashing (no chaining) on the
  // prefix code / next character combination. We do a variant of Knuth's
  // algorithm D (vol. 3, sec. 6.4) along with G. Knott's relatively-prime
  // secondary probe. Here, the modular division first probe is gives way
  // to a faster exclusive-or manipulation. Also do block compression with
  // an adaptive reset, whereby the code table is cleared when the compression
  // ratio decreases, but after the table fills. The variable-length output
  // codes are re-sized at this point, and a special CLEAR code is generated
  // for the decompressor. Late addition: construct the table according to
  // file size for noticeable speed improvement on small files. Please direct
  // questions about this implementation to ames!jaw.
  int g_init_bits, ClearCode, EOFCode;
  int remaining;
  int curPixel;
  int n_bits;

  LZWEncoder(int width, int height, char* pixels, int colorDepth);

  ~LZWEncoder();

  void encode(ByteArray &outs);

  void compress(int init_bits, ByteArray &outs);

  // Flush the packet to disk, and reset the accumulator
  void flush_char(ByteArray &outs);

  // Add a character to the end of the current packet, and if it is 254
  // characters, flush the packet to disk.
  void char_out(int c, ByteArray &outs);

  int MAXCODE(int n_bits);

  // Clear out the hash table
  // table clear for block compress
  void cl_block(ByteArray &outs);

  // Reset code table
  void cl_hash(int hsize);

  // Return the next pixel from the image
  int nextPixel();

  void output(int code, ByteArray &outs);
};
} // namespace gifencoder

#endif