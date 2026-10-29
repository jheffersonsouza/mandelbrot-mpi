#include <mpi.h>
#include "X11Display.h"
#include "Palette.h"
#include "Mandel.h"
#include "Params.h"
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

using namespace std;

// tag = bloco + 1, tag 0 = fim
const int TAG_DONE = 0;

// Com 2 blocos na fila, o worker não fica parado esperando a rede.
const int QUEUE_SIZE = 2;

int firstLine(const Params& p, int block)
{
   return block * p.blockLines;
}

int linesIn(const Params& p, int block)
{
   return min(p.blockLines, p.height - firstLine(p, block));
}

// Sem blocos sobrando, só manda o fim quando o worker já devolveu tudo.
void sendWork(const Params& p, int worker, int& nextBlock, int numBlocks, vector<int>& queued)
{
   if (nextBlock < numBlocks)
   {
      MPI_Send(NULL, 0, MPI_INT, worker, nextBlock + 1, MPI_COMM_WORLD);
      if (p.events)
         printf("@sent block=%d worker=%d\n", nextBlock, worker);
      nextBlock++;
      queued[worker]++;
   }
   else if (queued[worker] == 0)
      MPI_Send(NULL, 0, MPI_INT, worker, TAG_DONE, MPI_COMM_WORLD);
}


void worker(const Params& p)
{
   Mandel m;
   vector<unsigned short> iters(p.blockLines * p.width);
   MPI_Status status;
   int block;

   while (true)
   {
      MPI_Recv(NULL, 0, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
      if (status.MPI_TAG == TAG_DONE)
         break;

      block = status.MPI_TAG - 1;
      m.calcLines(iters.data(), firstLine(p, block), linesIn(p, block),
                  p.width, p.height, p.zmin, p.zmax, p.maxIter);
      MPI_Send(iters.data(), linesIn(p, block) * p.width, MPI_UNSIGNED_SHORT,
               0, status.MPI_TAG, MPI_COMM_WORLD);
   }
}


void master(const Params& p, int numProcs, X11Display& screen, bool hasWindow,
            vector<unsigned short>& image, vector<int>& blockOwner)
{
   int numBlocks = (p.height + p.blockLines - 1) / p.blockLines;
   int nextBlock = 0;
   int received = 0;
   int worker, block;
   vector<int> queued(numProcs, 0);   // blocos na fila de cada worker
   Mandel m;
   RGBPalette palette(0.4, 0.95, 0.65, 0.45, 0.5, 0.5, 0.5);
   MPI_Status status;

   if (p.events)
      printf("@start blocks=%d lines=%d width=%d height=%d iter=%d workers=%d\n",
             numBlocks, p.blockLines, p.width, p.height, p.maxIter, numProcs - 1);

   double start = MPI_Wtime();

   // Enche a fila de cada worker.
   for (int i = 0; i < QUEUE_SIZE; i++)
      for (worker = 1; worker < numProcs; worker++)
         if (nextBlock < numBlocks)
            sendWork(p, worker, nextBlock, numBlocks, queued);

   // Se tem mais workers que blocos, quem ficou sem bloco já recebe o fim.
   for (worker = 1; worker < numProcs; worker++)
      if (queued[worker] == 0)
         sendWork(p, worker, nextBlock, numBlocks, queued);

   while (received < numBlocks)
   {
      // Olha a mensagem antes de receber, para gravar o bloco direto no lugar certo.
      MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
      worker = status.MPI_SOURCE;
      block = status.MPI_TAG - 1;
      MPI_Recv(&image[firstLine(p, block) * p.width], linesIn(p, block) * p.width,
               MPI_UNSIGNED_SHORT, worker, status.MPI_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      queued[worker]--;
      received++;
      blockOwner[block] = worker;
      if (p.events)
         printf("@received block=%d worker=%d\n", block, worker);

      // Repõe a fila antes de desenhar, para o worker não esperar.
      sendWork(p, worker, nextBlock, numBlocks, queued);

      if (hasWindow)
      {
         m.showLines(screen, palette, &image[firstLine(p, block) * p.width],
                     firstLine(p, block), linesIn(p, block), p.maxIter);
         screen.flushLines(firstLine(p, block), linesIn(p, block));
      }
   }

   double seconds = MPI_Wtime() - start;
   if (p.events)
      printf("@done seconds=%.3f\n", seconds);
   printf("Distributed compute time: %.3f s (%dx%d, %d iterations, %d blocks of %d lines, %d workers)\n",
          seconds, p.width, p.height, p.maxIter, numBlocks, p.blockLines, numProcs - 1);
}


void summary(const Params& p, int numProcs, const vector<string>& rankHost,
             const vector<int>& blockOwner)
{
   int numBlocks = blockOwner.size();
   vector<string> hosts;
   vector<int> hostOf(numProcs);

   for (int r = 0; r < numProcs; r++)
   {
      int k = find(hosts.begin(), hosts.end(), rankHost[r]) - hosts.begin();
      if (k == (int) hosts.size())
         hosts.push_back(rankHost[r]);
      hostOf[r] = k;
   }

   printf("\n%-20s %9s %7s %7s %8s\n", "Host", "Workers", "Blocks", "Lines", "Image");
   for (int k = 0; k < (int) hosts.size(); k++)
   {
      int workers = 0, blocks = 0, lines = 0;
      for (int r = 1; r < numProcs; r++)
         if (hostOf[r] == k)
            workers++;
      for (int b = 0; b < numBlocks; b++)
         if (hostOf[blockOwner[b]] == k)
         {
            blocks++;
            lines += linesIn(p, b);
         }
      string name = hosts[k] + (k == hostOf[0] ? " (master)" : "");
      printf("%-20s %9d %7d %7d %7.1f%%\n", name.c_str(), workers, blocks, lines,
             100.0 * lines / p.height);
   }
}


int
main(int argc, char *argv[])
{
   int rank, numProcs, length;
   char name[MPI_MAX_PROCESSOR_NAME] = "";
   Params p;
   X11Display screen;
   bool hasWindow = false;

   MPI_Init(&argc, &argv);
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);
   MPI_Comm_size(MPI_COMM_WORLD, &numProcs);

   if (!parseParams(argc, argv, p) || numProcs < 2)
   {
      if (rank == 0)
      {
         fprintf(stderr, USAGE, argv[0]);
         fprintf(stderr, "Needs at least 2 processes.\n");
      }
      MPI_Finalize();
      return 1;
   }

   if (p.events)
      setvbuf(stdout, NULL, _IOLBF, 0);

   MPI_Get_processor_name(name, &length);
   vector<char> names(numProcs * MPI_MAX_PROCESSOR_NAME);
   MPI_Gather(name, MPI_MAX_PROCESSOR_NAME, MPI_CHAR,
              names.data(), MPI_MAX_PROCESSOR_NAME, MPI_CHAR, 0, MPI_COMM_WORLD);

   if (rank == 0)
   {
      vector<string> rankHost;
      for (int r = 0; r < numProcs; r++)
      {
         rankHost.push_back(&names[r * MPI_MAX_PROCESSOR_NAME]);
         if (p.events)
            printf("@rank rank=%d host=%s\n", r, rankHost[r].c_str());
      }

      vector<unsigned short> image(p.width * p.height);
      int numBlocks = (p.height + p.blockLines - 1) / p.blockLines;
      vector<int> blockOwner(numBlocks);

      if (p.window)
      {
         hasWindow = screen.open(p.width, p.height, "Mandelbrot with MPI");
         if (!hasWindow)
            printf("Could not open the window.\n");
      }

      master(p, numProcs, screen, hasWindow, image, blockOwner);
      summary(p, numProcs, rankHost, blockOwner);
   }
   else
      worker(p);

   MPI_Finalize();

   if (hasWindow)
   {
      printf("Close the window (or press q in it) to finish.\n");
      fflush(stdout);
      screen.waitClose();
      screen.close();
   }
   return 0;
}
