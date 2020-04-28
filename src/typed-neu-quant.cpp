#include "typed-neu-quant.h"
#include "iostream"
#include "cstdlib"
#include "vector"

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
TypedNeuQuant::TypedNeuQuant(vector<int> p, int s) : pixels(p), samplefac(s){};

void TypedNeuQuant::init()
{
  int i, v;
  for (i = 0; i < netsize; i++)
  {
    v = (i << (int(netbiasshift) + 8)) / netsize;

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
    network[i][0] = int(network[i][0]) >> netbiasshift;
    network[i][1] = int(network[i][1]) >> netbiasshift;
    network[i][2] = int(network[i][2]) >> netbiasshift;
    network[i][3] = i; // record color number
  }
};
/*
    Private Method: altersingle

    moves neuron *i* towards biased (b,g,r) by factor *alpha*
  */
void TypedNeuQuant::altersingle(int alpha, int i, int b, int g, int r)
{
  network[i][0] -= (alpha * (network[i][0] - b)) / initalpha;
  network[i][1] -= (alpha * (network[i][1] - g)) / initalpha;
  network[i][2] -= (alpha * (network[i][2] - r)) / initalpha;
};

/*
    Private Method: alterneigh

    moves neurons in *radius* around index *i* towards biased (b,g,r) by factor *alpha*
  */
void TypedNeuQuant::alterneigh(int radius, int i, int b, int g, int r)
{
  int lo = abs(i - radius);
  int hi = i + radius < netsize ? i + radius : netsize;

  int j = i + 1;
  int k = i - 1;
  int m = 1;

  int a;
  while ((j < hi) || (k > lo))
  {
    a = radpower[m++];

    if (j < hi)
    {
      int p = j++;
      network[p][0] -= (a * (network[p][0] - b)) / alpharadbias;
      network[p][1] -= (a * (network[p][1] - g)) / alpharadbias;
      network[p][2] -= (a * (network[p][2] - r)) / alpharadbias;
    }

    if (k > lo)
    {
      int p = k--;
      network[p][0] -= (a * (network[p][0] - b)) / alpharadbias;
      network[p][1] -= (a * (network[p][1] - g)) / alpharadbias;
      network[p][2] -= (a * (network[p][2] - r)) / alpharadbias;
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

  int bestd = ~(1 << 31);
  int bestbiasd = bestd;
  int bestpos = -1;
  int bestbiaspos = bestpos;

  int i, dist, biasdist, betafreq;
  for (i = 0; i < netsize; i++)
  {
    dist = abs(network[i][0] - b) + abs(network[i][1] - g) + abs(network[i][2] - r);
    if (dist < bestd)
    {
      bestd = dist;
      bestpos = i;
    }

    biasdist = dist - ((bias[i]) >> (intbiasshift - netbiasshift));
    if (biasdist < bestbiasd)
    {
      bestbiasd = biasdist;
      bestbiaspos = i;
    }

    betafreq = (freq[i] >> betashift);
    freq[i] -= betafreq;
    bias[i] += (betafreq << gammashift);
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
  int i, j, smallpos, smallval, previouscol = 0, startpos = 0;
  for (i = 0; i < netsize; i++)
  {
    smallpos = i;
    smallval = network[i][1]; // index on g
    // find smallest in i..netsize-1
    for (j = i + 1; j < netsize; j++)
    {
      if (network[j][1] < smallval)
      { // index on g
        smallpos = j;
        smallval = network[j][1]; // index on g
      }
    }
    // swap p (i) and q (smallpos) entries
    if (i != smallpos)
    {
      j = network[smallpos][0];
      network[smallpos][0] = network[i][0];
      network[i][0] = j;
      j = network[smallpos][1];
      network[smallpos][1] = network[i][1];
      network[i][1] = j;
      j = network[smallpos][2];
      network[smallpos][2] = network[i][2];
      network[i][2] = j;
      j = network[smallpos][3];
      network[smallpos][3] = network[i][3];
      network[i][3] = j;
    }
    // smallval entry is now in position i

    if (smallval != previouscol)
    {
      netindex[previouscol] = (startpos + i) >> 1;
      for (j = previouscol + 1; j < smallval; j++)
        netindex[j] = i;
      previouscol = smallval;
      startpos = i;
    }
  }
  netindex[previouscol] = (startpos + maxnetpos) >> 1;
  for (j = previouscol + 1; j < 256; j++)
    netindex[j] = maxnetpos; // really 256
};
/*
    Private Method: inxsearch

    searches for BGR values 0..255 and returns a color index
  */
int TypedNeuQuant::inxsearch(int b, int g, int r)
{
  int a, dist;

  int bestd = 1000; // biggest possible dist is 256*3
  int best = -1;

  int i = netindex[g]; // index on g
  int j = i - 1;       // start at netindex[g] and work outwards

  while ((i < netsize) || (j >= 0))
  {
    if (i < netsize)
    {
      dist = network[i][1] - g; // inx key
      if (dist >= bestd)
        i = netsize; // stop iter
      else
      {
        i++;
        if (dist < 0)
          dist = -dist;
        a = network[i][0] - b;
        if (a < 0)
          a = -a;
        dist += a;
        if (dist < bestd)
        {
          a = network[i][2] - r;
          if (a < 0)
            a = -a;
          dist += a;
          if (dist < bestd)
          {
            bestd = dist;
            best = network[i][3];
          }
        }
      }
    }
    if (j >= 0)
    {
      dist = g - network[j][1]; // inx key - reverse dif
      if (dist >= bestd)
        j = -1; // stop iter
      else
      {
        j--;
        if (dist < 0)
          dist = -dist;
        a = network[j][0] - b;
        if (a < 0)
          a = -a;
        dist += a;
        if (dist < bestd)
        {
          a = network[j][2] - r;
          if (a < 0)
            a = -a;
          dist += a;
          if (dist < bestd)
          {
            bestd = dist;
            best = network[j][3];
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

  int lengthcount = pixels.size();
  int alphadec = 30 + ((samplefac - 1) / 3);
  int samplepixels = lengthcount / (3 * samplefac);
  int delta = ~~(samplepixels / ncycles);
  int alpha = initalpha;
  int radius = initradius;

  int rad = radius >> radiusbiasshift;

  if (rad <= 1)
    rad = 0;
  for (i = 0; i < rad; i++)
    radpower[i] = alpha * (((rad * rad - i * i) * radbias) / (rad * rad));

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
    if (rad != 0)
    {
      alterneigh(rad, j, b, g, r); // alter neighbours
    }

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
      rad = radius >> radiusbiasshift;

      if (rad <= 1)
        rad = 0;
      for (j = 0; j < rad; j++)
        radpower[j] = alpha * (((rad * rad - j * j) * radbias) / (rad * rad));
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
vector<int> TypedNeuQuant::getColormap()
{
  vector<int> map;
  int index[netsize];

  for (int i = 0; i < netsize; i++)
    index[network[i][3]] = i;

  for (int l = 0; l < netsize; l++)
  {
    int j = index[l];
    map.push_back(network[j][0]);
    map.push_back(network[j][1]);
    map.push_back(network[j][2]);
  }
  return map;
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