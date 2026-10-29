#ifndef _PALETTE_H
#define _PALETTE_H

#include "Color.h"

/**
 Classe abstrata que representa uma paleta de cores. 
 Basicamente, esta classe define uma interface para se obter uma cor da paleta.
 O cálculo necessário para se obter a cor deve ser definido nas classes derivadas.
 */
class Palette
{
   public:
      /**
        Retorna a cor correspondente ao número n (n deve estar entre 0 e 1).
        */
      virtual Color getColor(float n) const = 0;
};


/**
 Paleta onde cada componente (R,G,B) da cor é calculado por uma senóide 
 de frequência f e fase p.
 */
class RGBPalette : public Palette
{
   public:
      /**
        Construtor.
        */   
      RGBPalette(float e,
                 float freq_r, float freq_g, float freq_b,
                 float phas_r, float phas_g, float phas_b);
      
      /**
        Retorna a cor correspondente ao número n (n deve estar entre 0 e 1).
        Cada componente (R,G,B) da cor é calculado por uma senóide 
        de frequência f e fase p. 
        */ 
      Color getColor(float n) const;

   private:
      float exp;        // expoente utilizado no cálculo da cor
      float f1, f2, f3; // frequências usadas no cálculo de cada componente da cor (r,g,b)
      float p1, p2, p3; // fases usadas no cálculo de cada componente da cor (r,g,b)

};


/**
 Paleta em tons de cinza.
 */
class GrayPalette : public Palette
{
   public:
      /**
        Construtor.
        */   
      GrayPalette(float e, float freq, float phas);
      
      /**
        Retorna o tom de cinza correspondente ao número n (n deve estar entre 0 e 1).
        */ 
      Color getColor(float n) const;

   private:
      float exp;  // expoente utilizado no cálculo da cor
      float f;    // frequência usada no cálculo dos componentes da cor
      float p;    // fase usada no cálculo dos componentes da cor

};


#endif // _PALETTE_H
