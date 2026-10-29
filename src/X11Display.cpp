#include "X11Display.h"
#include <cstdlib> // a função abs é declarada neste cabeçalho
#include <X11/keysym.h>

// Construtor da tela.
bool X11Display::open(unsigned int width, unsigned int height, const char *name)
{
   if (!createDisplay())
      return false;
   initColors();
   initWindow(width, height, name);
   initGC();
   initImage();

   displayWindow();
   return true;
}

void X11Display::close()
{
   XDestroyImage(image);
   XCloseDisplay(display);
}

// Inicializa display, estabelecendo conexão com servidor X.
bool X11Display::createDisplay()
{
   // Estabelece conexão com servidor X.
   // O argumento indica o nome do display (se for NULL, 
   // utiliza-se o valor da variável de ambiente DISPLAY).
   display = XOpenDisplay(NULL);
   if (display == NULL)
      return false;
       
   // Obtém o número do screen default deste display.  
   screen = DefaultScreen(display);
   return true;
}

// Calcula o número máximo de cores que podem ser mostradas na tela.
void X11Display::initColors()
{
   unsigned long white = WhitePixel(display, screen);
   unsigned long black = BlackPixel(display, screen);

   _colors = abs(black - white);
}

// Inicializa janela, definindo seus atributos.
void X11Display::initWindow(int width, int height, const char *name)
{
   unsigned long mask = 0L;
   XSetWindowAttributes attr;
    
   _width = width;
   _height = height;

   // Define atributos da janela a ser criada.
   attr.background_pixel = WhitePixel(display, screen);
   attr.backing_store = Always;
   attr.save_under = True;
   mask = CWBackPixel | CWBackingStore | CWSaveUnder;

   // Cria janela.    
   window = XCreateWindow(display, RootWindow(display, screen), 
                          0, 0,   // coordenadas do canto superior esquerdo
                          width, height,   // dimensões internas da janela 
                          0,   // número de pixels da borda da janela
                          0,   // profundidade da janela
                          InputOutput,   // classe da janela
                          (Visual *) CopyFromParent,   // tipo de visual da janela
                          mask,   // máscara que indica os atributos definidos em attr
                          &attr); // conjunto de atributos da janela

   // Define nome para a janela.
   XStoreName(display, window, name);

   // Eventos de redesenho, de teclado e de fechar a janela.
   XSelectInput(display, window, ExposureMask | KeyPressMask);
   closeMessage = XInternAtom(display, "WM_DELETE_WINDOW", False);
   XSetWMProtocols(display, window, &closeMessage, 1);
}

// Inicializa o contexto gráfico.
void X11Display::initGC()
{
   gc = XCreateGC(display, window, 0, NULL);
   XSetBackground(display, gc, WhitePixel(display, screen));
   XSetForeground(display, gc, BlackPixel(display, screen));
   XSetLineAttributes(display, gc, 1, LineSolid, CapRound, JoinRound);
}

void X11Display::initImage()
{
   char *pixels = (char *) malloc(_width * _height * 4);
   image = XCreateImage(display, DefaultVisual(display, screen),
                        DefaultDepth(display, screen), ZPixmap, 0,
                        pixels, _width, _height, 32, 0);
}

// Mostra janela.
void X11Display::displayWindow()
{
   XEvent event;

   // Mapeia a janela no display para torná-la visível.
   XMapWindow(display, window);
   do {
      XNextEvent(display, &event);
   } while (event.type != Expose);
}

// Atualiza tela.
void X11Display::flush()
{
   flushLines(0, _height);
}

void X11Display::flushLines(int firstLine, int numLines)
{
   XPutImage(display, window, gc, image, 0, firstLine, 0, firstLine, _width, numLines);
   XFlush(display);
}

void X11Display::waitClose()
{
   XEvent event;
   KeySym key;

   while (true)
   {
      XNextEvent(display, &event);
      if (event.type == Expose)
         flush();
      else if (event.type == KeyPress)
      {
         key = XLookupKeysym(&event.xkey, 0);
         if (key == XK_q || key == XK_Escape)
            break;
      }
      else if (event.type == ClientMessage && (Atom) event.xclient.data.l[0] == closeMessage)
         break;
   }
}

// Mostra na tela o ponto de coordenadas (x,y), pintando-o com color.
void X11Display::drawPoint(int x, int y, unsigned long color)
{
   XPutPixel(image, x, y, color);
}

// Mostra na tela o ponto de coordenadas (x,y), pintando-o com color (RGB).
void X11Display::drawPoint(int x, int y, Color color)
{
   unsigned long c = X11Display::color(color.r(), color.g(), color.b());
   XPutPixel(image, x, y, c);
}

// Número máximo de cores na tela.
unsigned long X11Display::colors()
{
   return _colors;
}

// Largura da tela.
unsigned int X11Display::width()
{
   return _width;
}

// Altura da tela.
unsigned int X11Display::height()
{
   return _height;
}

#define AJEITA(x) (x < 0 ? 0 : (x > 1 ? 1 : x))
#define COMP(x, X) ((int)(AJEITA(x) * ((1<<X)-1)))
#define COR(r, g, b, R, G, B) ((COMP(r,R)<<(G+B)) | (COMP(g,G)<<B) | COMP(b,B))

// Calcula cor a partir de componentes vermelho, verde e azul
// (r, g e b devem estar entre 0 e 1).
unsigned long X11Display::color(float r, float g, float b)
{
    return COR(r, g, b, B_R, B_G, B_B);
}




