#include "Palette.h"
#include <cmath>

/**
 Construtor.
 */   
RGBPalette::RGBPalette(float e,
                       float freq_r, float freq_g, float freq_b,
                       float phas_r, float phas_g, float phas_b) :
   exp(e),
   f1(freq_r), f2(freq_g), f3(freq_b), 
   p1(phas_r), p2(phas_g), p3(phas_b)
{ }

/**
 Retorna a cor correspondente ao número n (n deve estar entre 0 e 1).
 Cada componente (R,G,B) da cor é calculado por uma senóide 
 de frequência f e fase p. 
 */ 
Color RGBPalette::getColor(float n) const
{
   //n = (n < 0 ? 0 : (n > 1 ? 1 : n));
   
   float ne = pow(n, exp); 
   
   float r = pow(sin(ne*5*3.14*f1+p1*10*3.14/2), 2);
   float g = pow(sin(ne*5*3.14*f2+p2*10*3.14/2), 2);
   float b = pow(sin(ne*5*3.14*f3+p3*10*3.14/2), 2);
   
   return Color(r, g, b);
}

/**
 Construtor.
 */   
GrayPalette::GrayPalette(float e, float freq, float phas) :
   exp(e), f(freq), p(phas)
{ }

/**
 Retorna o tom de cinza correspondente ao número n (n deve estar entre 0 e 1).
 */ 
Color GrayPalette::getColor(float n) const
{
   //n = (n < 0 ? 0 : (n > 1 ? 1 : n));
   
   float ne = pow(n, exp);    
   float r, g, b;
   r = g = b = pow(sin(ne*5*3.14*f+p*10*3.14/2), 2);

   return Color(r, g, b);
}
