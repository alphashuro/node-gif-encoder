#ifndef TNEUQUANT_H
#define TNEUQUANT_H

#include "boost/compute/container/vector.hpp"
#include "valarray"

namespace gifencoder
{
class TypedNeuQuant
{
public:
  static const int netsize = 256; // number of colors used
  char*& pixels;
  int pixLen;
  int samplefac;

  int ncycles = 100; // number of learning cycles
  int maxnetpos = netsize - 1;

  // defs for freq and bias
  int netbiasshift = 4;  // bias for colour values
  int intbiasshift = 16; // bias for fractions
  int intbias = (1 << intbiasshift);
  int gammashift = 10;
  int gamma = (1 << gammashift);
  int betashift = 10;
  int beta = (intbias >> betashift); /* beta = 1/1024 */
  int betagamma = (intbias << (gammashift - betashift));

  // defs for decreasing radius factor
  int initrad = (netsize >> 3); // for 256 cols, radius starts
  int radiusbiasshift = 6;      // at 32.0 biased by 6 bits
  int radiusbias = (1 << radiusbiasshift);
  int initradius = (initrad * radiusbias); //and decreases by a
  int radiusdec = 30;                      // factor of 1/30 each cycle

  // defs for decreasing alpha factor
  int alphabiasshift = 10; // alpha starts at 1.0
  int initalpha = (1 << alphabiasshift);
  int alphadec; // biased by 10 bits

  /* radbias and alpharadbias used for radpower calculation */
  int radbiasshift = 8;
  int radbias = (1 << radbiasshift);
  int alpharadbshift = (alphabiasshift + radbiasshift);
  double alpharadbias = (1 << alpharadbshift);

  // four primes near 500 - assume no image has a length so large that it is
  // divisible by all four primes
  int prime1 = 499;
  int prime2 = 491;
  int prime3 = 487;
  int prime4 = 503;
  int minpicturebytes = (3 * prime4);

  std::valarray<double> network_0; // int[netsize][4]
  std::valarray<double> network_1; // int[netsize][4]
  std::valarray<double> network_2; // int[netsize][4]
  std::valarray<double> network_3; // int[netsize][4]
  int netindex[256]; // for network lookup - really 256

  // bias and freq arrays for learning
  double bias[netsize];
  double freq[netsize];
  double radpower[netsize >> 3];
  
  TypedNeuQuant(char*&, int, int);

  void init();
  void unbiasnet();
  void altersingle(double, int, double, double, double);
  void alterneigh(double, int, double, double, double);
  int contest(int, int, int);
  void inxbuild();
  int inxsearch(int, int, int);
  void learn();

  void buildColormap();
  void getColormap(std::array<int, netsize * 3> &map);
  int lookupRGB(int, int, int);
};
} // namespace gifencoder

#endif