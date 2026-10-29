#ifndef _MANDEL_H
#define _MANDEL_H

#include <complex> // classes de números complexos fazem parte da biblioteca padrão C++

#define dcomplex std::complex<double>

/**
 Função genérica que calcula o quadrado de um número complexo.
 */
template <typename Tc> 
Tc sqr(const Tc& a) 
{ 
   return Tc (a.real() * a.real() - a.imag() * a.imag(), 
              2 * a.real() * a.imag()); 
} 

/**
 Classe que encapsula a geração de fractais de Mandelbrot.
 */
class Mandel
{

   public:
   
      /**
        Calcula o valor de um ponto no plano complexo.
        */
      unsigned int calcPoint(dcomplex& c, int maxiter);
      
      // Calcula um bloco de linhas e guarda em iter as iterações de cada ponto.
      void calcLines(unsigned short *iter, int firstLine, int numLines,
         int width, int height, const dcomplex& zmin, const dcomplex& zmax, int maxiter);

      // Pinta na tela as linhas já calculadas.
      template<typename Td, typename Tp>
      void showLines(Td& d, const Tp& p, const unsigned short *iter,
         int firstLine, int numLines, int maxiter);
               
   private:
      // Quadrado do raio do plano complexo de Mandelbrot
      static constexpr double Raio2 = 4.0;   
};

/**
 Calcula o valor de um ponto no plano complexo.
 */
unsigned int
Mandel::calcPoint(dcomplex& c, int maxiter)
{
   int iter = 0;
   dcomplex z(0.0, 0.0);
   dcomplex z2; // quadrado de z
   double m2; // quadrado da magnitude de z

   do {
      z = sqr(z) + c;
      m2 = norm(z);
      iter++;
   } while ((m2 < Raio2) && (iter < maxiter));
   return iter;
}


void
Mandel::calcLines(unsigned short *iter, int firstLine, int numLines,
   int width, int height, const dcomplex& zmin, const dcomplex& zmax, int maxiter)
{
   int x, y;
   dcomplex c;

   double dx = (zmax.real() - zmin.real()) / (double) width;
   double dy = (zmax.imag() - zmin.imag()) / (double) height;

   for (y = firstLine; y < firstLine + numLines; y++)
   {
      for (x = 0; x < width; x++)
      {
         // mapeia (x,y) no plano complexo
         c = dcomplex((double) (zmin.real() + ((double) x * dx)), 
                      (double) (zmin.imag() + ((double) y * dy)));

         // calcula valor do fractal no ponto c
         iter[(y - firstLine) * width + x] = calcPoint(c, maxiter);
      }
   }
}

/**
 Pinta na tela os pontos já calculados.
 Usou-se templates (polimorfismo estático) para permitir que esta função possa
 ser reutilizada para diferentes classes que representam telas e paletas.
 A vinculação de tipos aos argumentos da função abaixo ocorre no momento da compilação
 (note que o código abaixo precisa ser compilado para cada combinação de argumentos, por
 isso a implementação está num arquivo com extensão ".h").
 */   

template<typename Td, typename Tp>
void
Mandel::showLines(Td& d, const Tp& p, const unsigned short *iter,
   int firstLine, int numLines, int maxiter)
{
   int x, y;
   int width = d.width();

   for (y = firstLine; y < firstLine + numLines; y++)
   {
      for (x = 0; x < width; x++)
      {
         // mostra ponto na tela
         d.drawPoint(x, y, p.getColor((float) iter[(y - firstLine) * width + x] / maxiter));
      }
   }
}



#endif // _MANDEL_H
