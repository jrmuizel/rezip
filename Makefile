CXXFLAGS=-Wall -g
all: rezip gzip

rezip: rezip.cc rezip.h
	g++ $(CXXFLAGS)   rezip.cc  -o rezip
gzip: gzip.cc rezip.h
	g++ $(CXXFLAGS)   gzip.cc  -o gzip
