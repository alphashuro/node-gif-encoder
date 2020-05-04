#include "typed-neu-quant.h"
#include "iostream"
#include "cstdlib"
#include "cmath"
#include <array>

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
pixLen(pixLen)
{};

void TypedNeuQuant::init()
{
  for (int i = 0; i < netsize; i++)
  {
    double v = (i << (netbiasshift + 8)) / netsize;

    network[i][0] = v;
    network[i][1] = v;
    network[i][2] = v;
    network[i][3] = 0;

    freq[i] = intbias / netsize;
    bias[i] = 0;
  }
};

/*
    Private Method: unbiasnet

    unbiases network to give int values 0..255 and record position i to prepare for sort
  */
void TypedNeuQuant::unbiasnet()
{
  for (int i = 0; i < netsize; i++)
  {
    auto p = network[i];

    network[i][0] = int(p[0]) >> netbiasshift;
    network[i][1] = int(p[1]) >> netbiasshift;
    network[i][2] = int(p[2]) >> netbiasshift;
    network[i][3] = i; // record color number
  }
};
/*
    Private Method: altersingle

    moves neuron *i* towards biased (b,g,r) by factor *alpha*
  */
void TypedNeuQuant::altersingle(double alpha, int i, double b, double g, double r)
{
  auto p = network[i];

  auto res1 = p[0] - ((alpha * (p[0] - b)) / initalpha);
  auto res2 = p[1] - ((alpha * (p[1] - g)) / initalpha);
  auto res3 = p[2] - ((alpha * (p[2] - r)) / initalpha);

  network[i][0] = res1;
  network[i][1] = res2;
  network[i][2] = res3;
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
      network[p][0] -= (a * (network[p][0] - b)) / alpharadbias; 
      network[p][1] -= (a * (network[p][1] - g)) / alpharadbias; 
      network[p][2] -= (a * (network[p][2] - r)) / alpharadbias; 
      // auto p = network[j++];
      // p[0] -= (a * (p[0] - b)) / alpharadbias;
      // p[1] -= (a * (p[1] - g)) / alpharadbias;
      // p[2] -= (a * (p[2] - r)) / alpharadbias;
    }

    if (k > lo)
    {
      int p = k--;
      network[p][0] -= (a * (network[p][0] - b)) / alpharadbias;
      network[p][1] -= (a * (network[p][1] - g)) / alpharadbias;
      network[p][2] -= (a * (network[p][2] - r)) / alpharadbias;
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
    auto n = network[i];

    dist = abs(n[0] - b) + abs(n[1] - g) + abs(n[2] - r);
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
    auto p = network[i];

    smallpos = i;
    smallval = p[1]; // index on g
    // find smallest in i..netsize-1
    for (int j = i + 1; j < netsize; j++)
    {
      auto q = network[j];

      if (q[1] < smallval)
      { // index on g
        smallpos = j;
        smallval = q[1]; // index on g
      }
    }

    auto q = network[smallpos];

    // swap p (i) and q (smallpos) entries
    if (i != smallpos)
    {
      double j;

      j = network[smallpos][0]; network[smallpos][0] = network[i][0]; network[i][0] = j;
      j = network[smallpos][1]; network[smallpos][1] = network[i][1]; network[i][1] = j;
      j = network[smallpos][2]; network[smallpos][2] = network[i][2]; network[i][2] = j;
      j = network[smallpos][3]; network[smallpos][3] = network[i][3]; network[i][3] = j;
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
      auto p = network[i];

      dist = network[i][1] - g; // inx key
      if (dist >= bestd)
        i = netsize; // stop iter
      else
      {
        i++;
        if (dist < 0)
          dist = -dist;

        a = p[0] - b;
        if (a < 0)
          a = -a;
        dist += a;
        if (dist < bestd)
        {
          a = p[2] - r;
          if (a < 0)
            a = -a;
          dist += a;
          if (dist < bestd)
          {
            bestd = dist;
            best = p[3];
          }
        }
      }
    }
    if (j >= 0)
    {
      auto p = network[j];

      dist = g - network[j][1]; // inx key - reverse dif
      if (dist >= bestd)
        j = -1; // stop iter
      else
      {
        j--;
        if (dist < 0)
          dist = -dist;
        a = p[0] - b;
        if (a < 0)
          a = -a;
        dist += a;
        if (dist < bestd)
        {
          a = p[2] - r;
          if (a < 0)
            a = -a;
          dist += a;
          if (dist < bestd)
          {
            bestd = dist;
            best = p[3];
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
  init();
  learn();
  unbiasnet();
  inxbuild();
};
/*
    Method: getColormap

    builds colormap from the index

    returns array in the format:

    >
    > [r, g, b, r, g, b, r, g, b, ..]
    >
  */
void TypedNeuQuant::getColormap(array<int, netsize * 3> & map)
{
  // int map[netsize * 3];
  int index[netsize];

  for (int i = 0; i < netsize; i++)
    index[int(network[i][3])] = i;

  for (int l = 0; l < netsize; l++)
  {
    int j = index[l];
    map[(l*3) + 0] = network[j][0];
    map[(l*3) + 1] = network[j][1];
    map[(l*3) + 2] = network[j][2];
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