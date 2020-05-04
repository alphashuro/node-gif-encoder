#include "typed-neu-quant.h"
#include "iostream"
#include "cstdlib"
#include "vector"
#include "cmath"

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
TypedNeuQuant::TypedNeuQuant(vector<char> p, int s) : pixels(p), samplefac(s){};

void TypedNeuQuant::init()
{
  network.clear();
  network.resize(netsize);
  netindex.clear();
  netindex.resize(256);
  bias.clear();
  bias.resize(netsize);
  freq.clear();
  freq.resize(netsize);
  radpower.clear();
  radpower.resize(netsize >> 3);

  for (int i = 0; i < netsize; i++)
  {
    double v = (i << (netbiasshift + 8)) / netsize;
    vector<double> temp{v, v, v, 0};

    network.at(i) = temp;

    freq.at(i) = intbias / netsize;
    bias.at(i) = 0;
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
    auto p = network.at(i);

    network.at(i).at(0) = int(p.at(0)) >> netbiasshift;
    network.at(i).at(1) = int(p.at(1)) >> netbiasshift;
    network.at(i).at(2) = int(p.at(2)) >> netbiasshift;
    network.at(i).at(3) = i; // record color number
  }
};
/*
    Private Method: altersingle

    moves neuron *i* towards biased (b,g,r) by factor *alpha*
  */
void TypedNeuQuant::altersingle(double alpha, int i, double b, double g, double r)
{
  auto p = network.at(i);

  auto res1 = p.at(0) - ((alpha * (p.at(0) - b)) / initalpha);
  auto res2 = p.at(1) - ((alpha * (p.at(1) - g)) / initalpha);
  auto res3 = p.at(2) - ((alpha * (p.at(2) - r)) / initalpha);

  network.at(i).at(0) = res1;
  network.at(i).at(1) = res2;
  network.at(i).at(2) = res3;
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
    auto a = radpower.at(m++);

    if (j < hi)
    {
      
      int p = j++;
      network.at(p).at(0) -= (a * (network.at(p).at(0) - b)) / alpharadbias; 
      network.at(p).at(1) -= (a * (network.at(p).at(1) - g)) / alpharadbias; 
      network.at(p).at(2) -= (a * (network.at(p).at(2) - r)) / alpharadbias; 
      // auto p = network.at(j++);
      // p.at(0) -= (a * (p[0] - b)) / alpharadbias;
      // p.at(1) -= (a * (p[1] - g)) / alpharadbias;
      // p.at(2) -= (a * (p[2] - r)) / alpharadbias;
    }

    if (k > lo)
    {
      int p = k--;
      network.at(p).at(0) -= (a * (network.at(p).at(0) - b)) / alpharadbias;
      network.at(p).at(1) -= (a * (network.at(p).at(1) - g)) / alpharadbias;
      network.at(p).at(2) -= (a * (network.at(p).at(2) - r)) / alpharadbias;
      // auto p = network.at(k--);
      // p.at(0) -= (a * (p[0] - b)) / alpharadbias;
      // p.at(1) -= (a * (p[1] - g)) / alpharadbias;
      // p.at(2) -= (a * (p[2] - r)) / alpharadbias;
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
      for frequently chosen neurons, freq.at(i) is high and bias[i] is negative
      bias[i] = gamma * ((1 / netsize) - freq.at(i))
    */

  double bestd = ~(1 << 31);
  double bestbiasd = bestd;
  int bestpos = -1;
  int bestbiaspos = bestpos;

  double i, dist, biasdist, betafreq;
  for (i = 0; i < netsize; i++)
  {
    auto n = network.at(i);

    dist = abs(n.at(0) - b) + abs(n.at(1) - g) + abs(n.at(2) - r);
    if (dist < bestd)
    {
      bestd = dist;
      bestpos = i;
    }

    biasdist = dist - (int(bias.at(i)) >> (intbiasshift - netbiasshift));
    if (biasdist < bestbiasd)
    {
      bestbiasd = biasdist;
      bestbiaspos = i;
    }

    betafreq = (int(freq.at(i)) >> betashift);
    freq.at(i) -= betafreq;
    bias.at(i) += (int(betafreq) << gammashift);
  }

  freq.at(bestpos) += beta;
  bias.at(bestpos) -= betagamma;

  return bestbiaspos;
};

/*
    Private Method: inxbuild

    sorts network and builds netindex[0..255]
  */
void TypedNeuQuant::inxbuild()
{
  double i, j, smallpos, smallval, previouscol = 0, startpos = 0;
  for (i = 0; i < netsize; i++)
  {
    auto p = network.at(i);

    smallpos = i;
    smallval = p.at(1); // index on g
    // find smallest in i..netsize-1
    for (j = i + 1; j < netsize; j++)
    {
      auto q = network.at(j);

      if (q.at(1) < smallval)
      { // index on g
        smallpos = j;
        smallval = q.at(1); // index on g
      }
    }

    auto q = network.at(smallpos);

    // swap p (i) and q (smallpos) entries
    if (i != smallpos)
    {
      network.at(i).swap(network.at(smallpos));

      // j = q.at(0); q[0] = p[0]; p[0] = j;
      // j = q.at(1); q[1] = p[1]; p[1] = j;
      // j = q.at(2); q[2] = p[2]; p[2] = j;
      // j = q.at(3); q[3] = p[3]; p[3] = j;
    }
    // smallval entry is now in position i

    if (smallval != previouscol)
    {
      netindex.at(previouscol) = int(startpos + i) >> 1;
      for (j = previouscol + 1; j < smallval; j++) {
        netindex.at(j) = i;
      }
      previouscol = smallval;
      startpos = i;
    }
  }
  netindex.at(previouscol) = int(startpos + maxnetpos) >> 1;
  for (j = previouscol + 1; j < 256; j++)
    netindex.at(j) = maxnetpos; // really 256
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

  int i = netindex.at(g); // index on g
  int j = i - 1;          // start at netindex.at(g) and work outwards

  while ((i < netsize) || (j >= 0))
  {
    if (i < netsize)
    {
      auto p = network.at(i);

      dist = network.at(i).at(1) - g; // inx key
      if (dist >= bestd)
        i = netsize; // stop iter
      else
      {
        i++;
        if (dist < 0)
          dist = -dist;

        a = p.at(0) - b;
        if (a < 0)
          a = -a;
        dist += a;
        if (dist < bestd)
        {
          a = p.at(2) - r;
          if (a < 0)
            a = -a;
          dist += a;
          if (dist < bestd)
          {
            bestd = dist;
            best = p.at(3);
          }
        }
      }
    }
    if (j >= 0)
    {
      auto p = network.at(j);

      dist = g - network.at(j).at(1); // inx key - reverse dif
      if (dist >= bestd)
        j = -1; // stop iter
      else
      {
        j--;
        if (dist < 0)
          dist = -dist;
        a = p.at(0) - b;
        if (a < 0)
          a = -a;
        dist += a;
        if (dist < bestd)
        {
          a = p.at(2) - r;
          if (a < 0)
            a = -a;
          dist += a;
          if (dist < bestd)
          {
            bestd = dist;
            best = p.at(3);
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
    radpower.at(i) = res;
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
        radpower.at(j) = alpha * ((((rad * rad) - (j * j)) * radbias) / (rad * rad));
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
    index[int(network.at(i).at(3))] = i;

  for (int l = 0; l < netsize; l++)
  {
    int j = index[l];
    map.push_back(network.at(j).at(0));
    map.push_back(network.at(j).at(1));
    map.push_back(network.at(j).at(2));
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