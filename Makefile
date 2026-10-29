CXX = mpicxx
CXXFLAGS = -Wall -O2 -Isrc
LDLIBS = -lX11
HEADERS = $(wildcard src/*.h)

all: bin/mandelbrot_mpi bin/mandelbrot_seq

bin/mandelbrot_mpi: src/mandelbrot_mpi.cpp src/X11Display.cpp src/Palette.cpp $(HEADERS) | bin
	$(CXX) $(CXXFLAGS) src/mandelbrot_mpi.cpp src/X11Display.cpp src/Palette.cpp -o $@ $(LDLIBS)

bin/mandelbrot_seq: src/mandelbrot_seq.cpp $(HEADERS) | bin
	g++ $(CXXFLAGS) src/mandelbrot_seq.cpp -o $@

bin:
	mkdir -p $@

clean:
	rm -rf bin

.PHONY: all clean
