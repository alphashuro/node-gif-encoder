#include "typed-neu-quant.h"
#include "iostream"
#include "cstdlib"
#include "cmath"
#include <array>
#include "boost/compute/container/vector.hpp"
#include "valarray"
#include "numeric"

using namespace std;

namespace gifencoder
{

/*
  Constructor: NeuQuant

  Arguments:

  pixels - array of pixels in RGB format
  samplefac - sampling factor 1 to 30 where lower is better quality

  >
  > pixels = [r, g, b, r, g, b, r, g, b, ..]
  >
*/
TypedNeuQuant::TypedNeuQuant(char*& p, int s, int pixLen) : 
pixels(p), 
samplefac(s), 
pixLen(pixLen),
network_0(netsize),
network_1(netsize),
network_2(netsize),
network_3(double(0), netsize)
{};

void TypedNeuQuant::init()
{
  iota(begin(network_0), end(network_0), 0);

  int bitshift = netbiasshift + 8;

  network_0 <<= bitshift;
  network_0 /= netsize;

  network_1 = network_0;
  network_2 = network_0;

  auto freq_val = intbias / netsize;

  for (int i = 0; i < netsize; i++)
  {
    freq[i] = freq_val;
    bias[i] = 0;
  }
};

/*
    Private Method: unbiasnet

    unbiases network to give int values 0..255 and record position i to prepare for sort
  */
void TypedNeuQuant::unbiasnet()
{
  // network_0 >>= double(netbiasshift);
  // network_1 >>= netbiasshift;
  // network_2 >>= netbiasshift;

  for (int i = 0; i < netsize; i++)
  {
    network_0[i] = int(network_0[i]) >> netbiasshift;
    network_1[i] = int(network_1[i]) >> netbiasshift;
    network_2[i] = int(network_2[i]) >> netbiasshift;
    network_3[i] = i; // record color number
  }
};
/*
    Private Method: altersingle

    moves neuron *i* towards biased (b,g,r) by factor *alpha*
  */
void TypedNeuQuant::altersingle(double alpha, int i, double b, double g, double r)
{
  auto res1 = network_0[i] - ((alpha * (network_0[i] - b)) / initalpha);
  auto res2 = network_1[i] - ((alpha * (network_1[i] - g)) / initalpha);
  auto res3 = network_2[i] - ((alpha * (network_2[i] - r)) / initalpha);

  network_0[i] = res1;
  network_1[i] = res2;
  network_2[i] = res3;
};

/*
    Private Method: alterneigh

    moves neurons in *radius* around index *i* towards biased (b,g,r) by factor *alpha*
  */
void TypedNeuQuant::alterneigh(double radius, int i, double b, double g, double r)
{
  int lo = abs(i - radius);
  int hi = i + radius < netsize ? i + radius : netsize;

  int j = i + 1;
  int k = i - 1;
  int m = 1;

  while ((j < hi) || (k > lo))
  {
    auto a = radpower[m++];

    if (j < hi)
    {
      
      int p = j++;
      network_0[p] -= (a * (network_0[p] - b)) / alpharadbias; 
      network_1[p] -= (a * (network_1[p] - g)) / alpharadbias; 
      network_2[p] -= (a * (network_2[p] - r)) / alpharadbias; 
      // auto p = network[j++];
      // p[0] -= (a * (p[0] - b)) / alpharadbias;
      // p[1] -= (a * (p[1] - g)) / alpharadbias;
      // p[2] -= (a * (p[2] - r)) / alpharadbias;
    }

    if (k > lo)
    {
      int p = k--;
      network_0[p] -= (a * (network_0[p] - b)) / alpharadbias;
      network_1[p] -= (a * (network_1[p] - g)) / alpharadbias;
      network_2[p] -= (a * (network_2[p] - r)) / alpharadbias;
      // auto p = network[k--];
      // p[0] -= (a * (p[0] - b)) / alpharadbias;
      // p[1] -= (a * (p[1] - g)) / alpharadbias;
      // p[2] -= (a * (p[2] - r)) / alpharadbias;
    }
  }
};
/*
    Private Method: contest

    searches for biased BGR values
  */
int TypedNeuQuant::contest(int b, int g, int r)
{
  /*
      finds closest neuron (min dist) and updates freq
      finds best neuron (min dist-bias) and returns position
      for frequently chosen neurons, freq[i] is high and bias[i] is negative
      bias[i] = gamma * ((1 / netsize) - freq[i])
    */

  double bestd = ~(1 << 31);
  double bestbiasd = bestd;
  int bestpos = -1;
  int bestbiaspos = bestpos;

  double dist, biasdist, betafreq;
  for (int i = 0; i < netsize; i++)
  {
    dist = abs(network_0[i] - b) + abs(network_1[i] - g) + abs(network_2[i] - r);
    if (dist < bestd)
    {
      bestd = dist;
      bestpos = i;
    }

    biasdist = dist - (int(bias[i]) >> (intbiasshift - netbiasshift));
    if (biasdist < bestbiasd)
    {
      bestbiasd = biasdist;
      bestbiaspos = i;
    }

    betafreq = (int(freq[i]) >> betashift);
    freq[i] -= betafreq;
    bias[i] += (int(betafreq) << gammashift);
  }

  freq[bestpos] += beta;
  bias[bestpos] -= betagamma;

  return bestbiaspos;
};

/*
    Private Method: inxbuild

    sorts network and builds netindex[0..255]
  */
void TypedNeuQuant::inxbuild()
{
  int smallpos;
  double smallval, previouscol = 0, startpos = 0;
  for (int i = 0; i < netsize; i++)
  {
    smallpos = i;
    smallval = network_1[i]; // index on g
    // find smallest in i..netsize-1
    for (int j = i + 1; j < netsize; j++)
    {
      if (network_1[j] < smallval)
      { // index on g
        smallpos = j;
        smallval = network_1[j]; // index on g
      }
    }

    // swap p (i) and q (smallpos) entries
    if (i != smallpos)
    {
      double j;

      j = network_0[smallpos]; network_0[smallpos] = network_0[i]; network_0[i] = j;
      j = network_1[smallpos]; network_1[smallpos] = network_1[i]; network_1[i] = j;
      j = network_2[smallpos]; network_2[smallpos] = network_2[i]; network_2[i] = j;
      j = network_3[smallpos]; network_3[smallpos] = network_3[i]; network_3[i] = j;
    }
    // smallval entry is now in position i

    if (smallval != previouscol)
    {
      netindex[int(previouscol)] = int(startpos + i) >> 1;
      for (int j = previouscol + 1; j < smallval; j++) {
        netindex[j] = i;
      }
      previouscol = smallval;
      startpos = i;
    }
  }
  netindex[int(previouscol)] = int(startpos + maxnetpos) >> 1;
  for (int j = previouscol + 1; j < 256; j++)
    netindex[j] = maxnetpos; // really 256
};
/*
    Private Method: inxsearch

    searches for BGR values 0..255 and returns a color index
  */
int TypedNeuQuant::inxsearch(int b, int g, int r)
{
  double a, dist;

  int bestd = 1000; // biggest possible dist is 256*3
  int best = -1;

  int i = netindex[g]; // index on g
  int j = i - 1;          // start at netindex[g] and work outwards

  while ((i < netsize) || (j >= 0))
  {
    if (i < netsize)
    {
      dist = network_1[i] - g; // inx key
      if (dist >= bestd)
        i = netsize; // stop iter
      else
      {
        i++;
        if (dist < 0)
          dist = -dist;

        a = network_0[i] - b;
        if (a < 0)
          a = -a;
        dist += a;
        if (dist < bestd)
        {
          a = network_2[i] - r;
          if (a < 0)
            a = -a;
          dist += a;
          if (dist < bestd)
          {
            bestd = dist;
            best = network_3[i];
          }
        }
      }
    }
    if (j >= 0)
    {
      dist = g - network_1[j]; // inx key - reverse dif
      if (dist >= bestd)
        j = -1; // stop iter
      else
      {
        j--;
        if (dist < 0)
          dist = -dist;
        a = network_0[j] - b;
        if (a < 0)
          a = -a;
        dist += a;
        if (dist < bestd)
        {
          a = network_2[j] - r;
          if (a < 0)
            a = -a;
          dist += a;
          if (dist < bestd)
          {
            bestd = dist;
            best = network_3[j];
          }
        }
      }
    }
  }

  return best;
};

/*
    Private Method: learn

    "Main Learning Loop"
  */
void TypedNeuQuant::learn()
{
  int i;

  int lengthcount = pixLen;
  int alphadec = 30 + ((samplefac - 1) / 3);
  int samplepixels = lengthcount / (3 * samplefac);
  int delta = ~~(samplepixels / ncycles);
  double alpha = initalpha;
  double radius = initradius;

  double rad = int(radius) >> radiusbiasshift;

  double radsquared = rad * rad;

  if (rad <= 1)
    rad = 0;
  for (i = 0; i < rad; i++)
  {
    int isqr = i * i;

    int res = alpha * double((radsquared - isqr) * radbias) / double(radsquared);
    radpower[i] = res;
  }

  int step;
  if (lengthcount < minpicturebytes)
  {
    samplefac = 1;
    step = 3;
  }
  else if ((lengthcount % prime1) != 0)
  {
    step = 3 * prime1;
  }
  else if ((lengthcount % prime2) != 0)
  {
    step = 3 * prime2;
  }
  else if ((lengthcount % prime3) != 0)
  {
    step = 3 * prime3;
  }
  else
  {
    step = 3 * prime4;
  }

  int b, g, r, j;
  int pix = 0; // current pixel

  i = 0;
  while (i < samplepixels)
  {
    b = int(pixels[pix] & 0xff) << netbiasshift;
    g = int(pixels[pix + 1] & 0xff) << netbiasshift;
    r = int(pixels[pix + 2] & 0xff) << netbiasshift;

    j = contest(b, g, r);

    altersingle(alpha, j, b, g, r);
    if (rad != 0) { alterneigh(rad, j, b, g, r); } // alter neighbours 

    pix += step;
    if (pix >= lengthcount)
      pix -= lengthcount;

    i++;

    if (delta == 0)
      delta = 1;
    if (i % delta == 0)
    {
      alpha -= alpha / alphadec;
      radius -= radius / radiusdec;
      rad = int(radius) >> radiusbiasshift;

      if (rad <= 1)
        rad = 0;
      for (j = 0; j < rad; j++)
        radpower[j] = alpha * ((((rad * rad) - (j * j)) * radbias) / (rad * rad));
    }
  }
};

/*
    Method: buildColormap

    1. initializes network
    2. trains it
    3. removes misconceptions
    4. builds colorindex
  */
void TypedNeuQuant::buildColormap()
{
  auto t1 = chrono::high_resolution_clock::now();
  init();
  auto t2 = chrono::high_resolution_clock::now();
  cout << "init: " << chrono::duration_cast<chrono::microseconds>(t2 - t1).count() << endl;

  t1 = chrono::high_resolution_clock::now();
  learn();
  t2 = chrono::high_resolution_clock::now();
  cout << "learn: " << chrono::duration_cast<chrono::microseconds>(t2 - t1).count() << endl;

  t1 = chrono::high_resolution_clock::now();
  unbiasnet();
  t2 = chrono::high_resolution_clock::now();
  cout << "unbiasnet: " << chrono::duration_cast<chrono::microseconds>(t2 - t1).count() << endl;

  t1 = chrono::high_resolution_clock::now();
  inxbuild();
  t2 = chrono::high_resolution_clock::now();
  cout << "inxbuild: " << chrono::duration_cast<chrono::microseconds>(t2 - t1).count() << endl;
};
/*
    Method: getColormap

    builds colormap from the index

    returns array in the format:

    >
    > [r, g, b, r, g, b, r, g, b, ..]
    >
  */
void TypedNeuQuant::getColormap(std::array<int, netsize * 3> & map)
{
  // int map[netsize * 3];
  int index[netsize];

  for (int i = 0; i < netsize; i++)
    index[int(network_3[i])] = i;

  for (int l = 0; l < netsize; l++)
  {
    int j = index[l];
    map[(l*3) + 0] = network_0[j];
    map[(l*3) + 1] = network_1[j];
    map[(l*3) + 2] = network_2[j];
  }
};
/*
    Method: lookupRGB

    looks for the closest *r*, *g*, *b* color in the map and
    returns its index
  */
int TypedNeuQuant::lookupRGB(int b, int g, int r)
{
  return inxsearch(int(b), int(g), int(r));
};
} // namespace gifencoder