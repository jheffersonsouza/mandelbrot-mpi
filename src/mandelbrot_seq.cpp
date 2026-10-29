#include "Mandel.h"
#include "Params.h"
#include <chrono>
#include <cstdio>
#include <vector>

using namespace std;

int
main(int argc, char *argv[])
{
   Params p;
   if (!parseParams(argc, argv, p))
   {
      fprintf(stderr, USAGE, argv[0]);
      return 1;
   }

   Mandel m;
   vector<unsigned short> image(p.width * p.height);

   chrono::steady_clock::time_point start = chrono::steady_clock::now();
   m.calcLines(image.data(), 0, p.height, p.width, p.height, p.zmin, p.zmax, p.maxIter);
   // Sem isto, o compilador pode descartar o cálculo, porque a imagem não é usada depois.
   asm volatile("" : : "g"(image.data()) : "memory");
   chrono::duration<double> seconds = chrono::steady_clock::now() - start;

   printf("Sequential compute time: %.3f s (%dx%d, %d iterations)\n",
          seconds.count(), p.width, p.height, p.maxIter);
   return 0;
}
