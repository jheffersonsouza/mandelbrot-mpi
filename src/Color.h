#ifndef _COLOR_H
#define _COLOR_H

/**
 Classe que representa uma cor no sistema RGB.
 */
class Color
{
   public:
      Color(float r, float g, float b) :
         _r(r), _g(g), _b(b) { }
      float r() { return _r; }
      float g() { return _g; }
      float b() { return _b; }
   private:
      float _r;
      float _g;
      float _b;
};

#endif   // _COLOR_H
