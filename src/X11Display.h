#ifndef _X11_DISPLAY_H
#define _X11_DISPLAY_H

#include "Color.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>

// O número de bits por pixel depende da configuração do servidor X.
// A definição abaixo deve ser ajustada para que as cores apareçam corretamente.
#define VINTE_E_QUATRO
#ifdef VINTE_E_QUATRO
#define B_R 8
#define B_G 8
#define B_B 8
#else
#define B_R 5
#define B_G 6
#define B_B 5
#endif

/**
 Classe que representa uma tela em XWindows.
 */
class X11Display {

   private:
   
      Display *display;
      unsigned int screen;
      unsigned int _width;
      unsigned int _height;
      unsigned long _colors;
      Window window;
      GC gc;
      XImage *image;
      Atom closeMessage;
      
   private:
   
      // Inicializa display, estabelecendo conexão com servidor X.
      bool createDisplay();
      
      // Calcula o número máximo de cores que podem ser mostradas na tela.
      void initColors();
      
      // Inicializa janela, definindo seus atributos.
      void initWindow(int width, int height, const char *name);
      
      // Inicializa o contexto gráfico.
      void initGC();

      // Cria a imagem na memória onde os pontos são pintados.
      void initImage();
      
      // Mostra janela.
      void displayWindow();
         
   public:

      bool open(unsigned int width, unsigned int height, const char *nome);
      
      void close();
      
      // Mostra na tela o ponto de coordenadas (x,y), pintando-o com color.
      void drawPoint(int x, int y, unsigned long color);

      // Mostra na tela o ponto de coordenadas (x,y), pintando-o com color (RGB).
      void drawPoint(int x, int y, Color color);
            
      // Número máximo de cores na tela.
      unsigned long colors();
      
      // Largura da tela.
      unsigned int width();
      
      // Altura da tela.
      unsigned int height();
      
      // Atualiza tela.
      void flush();

      // Atualiza na tela só as linhas indicadas.
      void flushLines(int firstLine, int numLines);

      // Espera a janela ser fechada.
      void waitClose();
      
      // Calcula cor a partir de componentes vermelho, verde e azul
      // (r, g e b devem estar entre 0 e 1).
      unsigned long color(float r, float g, float b);
      
   
};

#endif   // _X11_DISPLAY_H
