#ifndef _PARAMS_H
#define _PARAMS_H

#include "Mandel.h"
#include <cstdlib>
#include <cstring>

struct Params
{
   int width = 800;
   int height = 600;
   int maxIter = 20000;
   int blockLines = 8;
   bool window = true;
   bool events = false;
   dcomplex zmin = dcomplex(-0.482536, -0.635201);
   dcomplex zmax = dcomplex(-0.565928, -0.579588);
};

const char USAGE[] =
   "Usage: %s [--width N] [--height N] [--iter N] [--block N] [--full] [--no-window] [--events]\n";

inline bool parseParams(int argc, char *argv[], Params& p)
{
   for (int i = 1; i < argc; i++)
   {
      if (strcmp(argv[i], "--width") == 0 && i + 1 < argc)
         p.width = atoi(argv[++i]);
      else if (strcmp(argv[i], "--height") == 0 && i + 1 < argc)
         p.height = atoi(argv[++i]);
      else if (strcmp(argv[i], "--iter") == 0 && i + 1 < argc)
         p.maxIter = atoi(argv[++i]);
      else if (strcmp(argv[i], "--block") == 0 && i + 1 < argc)
         p.blockLines = atoi(argv[++i]);
      else if (strcmp(argv[i], "--no-window") == 0)
         p.window = false;
      else if (strcmp(argv[i], "--events") == 0)
         p.events = true;
      else if (strcmp(argv[i], "--full") == 0)
      {
         p.zmin = dcomplex(-1.5545173217428, -1.55451670625497);
         p.zmax = dcomplex(0.29958615131552137, 0.39958615593168024);
      }
      else
         return false;
   }

   // limites do unsigned short e da tag do MPI
   return p.width > 0 && p.height > 0 && p.height < 32767 &&
          p.maxIter > 0 && p.maxIter <= 65535 && p.blockLines > 0;
}

#endif   // _PARAMS_H
