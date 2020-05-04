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

#include "lzw-encoder.h"

using namespace std;

namespace gifencoder
{
// int gifencoder::EOF = -1;
int masks[] = {0x0000, 0x0001, 0x0003, 0x0007, 0x000F, 0x001F,
               0x003F, 0x007F, 0x00FF, 0x01FF, 0x03FF, 0x07FF,
               0x0FFF, 0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF};

LZWEncoder::LZWEncoder(int width, int height, char* p, int colorDepth) : width(width),
                                                                                height(height),
                                                                                pixels(p)
{
  initCodeSize = int(colorDepth < 2 ? 2 : colorDepth);
}

LZWEncoder::~LZWEncoder() {}

void LZWEncoder::encode(ByteArray &outs)

{
  outs.writeByte(initCodeSize); // write "initial code size" int
  remaining = width * height;   // reset navigation variables
  curPixel = 0;

  compress(int(initCodeSize) + 1, outs); // compress and write the pixel data
  outs.writeByte(int(0));                // write block terminator
}

void LZWEncoder::compress(int init_bits, ByteArray &outs)
{
  int fcode, c, i, ent, disp, hsize_reg, hshift;

  // Set up the globals: g_init_bits - initial number of bits
  g_init_bits = init_bits;

  // Set up the necessary values
  clear_flg = false;
  n_bits = g_init_bits;
  maxcode = MAXCODE(n_bits);

  ClearCode = 1 << (init_bits - 1);
  EOFCode = ClearCode + 1;
  free_ent = ClearCode + 2;

  a_count = 0; // clear packet

  ent = int(nextPixel());

  hshift = 0;
  for (fcode = HSIZE; fcode < 65536; fcode *= 2)
    ++hshift;
  hshift = 8 - hshift; // set hash code range bound
  hsize_reg = HSIZE;
  cl_hash(hsize_reg); // clear hash table

  output(ClearCode, outs);

  while ((c = int(nextPixel())) != EOF)
  {
    fcode = (c << BITS) + ent;
    i = (c << hshift) ^ ent; // xor hashing
    if (htab[i] == fcode)
    {
      ent = codetab[i];
      continue;
    }
    else if (htab[i] >= 0)
    {                       // non-empty slot
      disp = hsize_reg - i; // secondary hash (after G. Knott)
      if (i == 0)
        disp = 1;
      do
      {
        if ((i -= disp) < 0)
          i += hsize_reg;
        if (htab[i] == fcode)
        {
          ent = codetab[i];
          goto outer_loop;
        }
      } while (htab[i] >= 0);
    }
    output(ent, outs);
    ent = c;
    if (free_ent < 1 << BITS)
    {
      codetab[i] = free_ent++; // code -> hashtable
      htab[i] = fcode;
    }
    else
    {
      cl_block(outs);
    }

  outer_loop:;
  }

  // Put out the final code.
  output(ent, outs);
  output(EOFCode, outs);
}

// Flush the packet to disk, and reset the accumulator
void LZWEncoder::flush_char(ByteArray &outs)
{
  if (a_count > 0)
  {
    outs.writeByte(int(a_count));
    outs.writeBytes(accum, 0, a_count);
    a_count = 0;
  }
}

// Add a character to the end of the current packet, and if it is 254
// characters, flush the packet to disk.
void LZWEncoder::char_out(int c, ByteArray &outs)
{
  accum[a_count++] = c;
  if (a_count >= 254)
  {
    flush_char(outs);
  }
}

int LZWEncoder::MAXCODE(int n_bits)
{
  return (1 << n_bits) - 1;
}

// Clear out the hash table
// table clear for block compress
void LZWEncoder::cl_block(ByteArray &outs)
{
  cl_hash(HSIZE);
  free_ent = ClearCode + 2;
  clear_flg = true;
  output(ClearCode, outs);
}

// Reset code table
void LZWEncoder::cl_hash(int hsize)
{
  for (int i = 0; i < hsize; ++i)
    htab[i] = -1;
}

// Return the next pixel from the image
int LZWEncoder::nextPixel()
{
  if (remaining == 0)
    return int(EOF);
  --remaining;
  int pix = pixels[curPixel++];
  return int(int(pix) & 0xff);
}

void LZWEncoder::output(int code, ByteArray &outs)
{
  cur_accum &= masks[cur_bits];

  if (cur_bits > 0)
    cur_accum |= (code << cur_bits);
  else
    cur_accum = code;

  cur_bits += n_bits;

  while (cur_bits >= 8)
  {
    char_out(cur_accum & 0xff, outs);
    cur_accum >>= 8;
    cur_bits -= 8;
  }

  // If the next entry is going to be too big for the code size,
  // then increase it, if possible.
  if (free_ent > maxcode || clear_flg)
  {
    if (clear_flg)
    {
      maxcode = MAXCODE(n_bits = g_init_bits);
      clear_flg = false;
    }
    else
    {
      ++n_bits;
      if (n_bits == BITS)
        maxcode = 1 << BITS;
      else
        maxcode = MAXCODE(n_bits);
    }
  }

  if (code == EOFCode)
  {
    // At EOF, write the rest of the buffer.
    while (cur_bits > 0)
    {
      char_out(int(cur_accum & 0xff), outs);
      cur_accum >>= 8;
      cur_bits -= 8;
    }
    flush_char(outs);
  }
}

} // namespace gifencoder
