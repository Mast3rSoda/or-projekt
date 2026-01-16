CXX = g++
CXXFLAGS = -O3 -fopenmp -march=native

TARGETS = bin/seq bin/par1 bin/par2 bin/par2g

.PHONY: all clean gif

all: $(TARGETS)

seq: seq.cpp
	$(CXX) $(CXXFLAGS) -o bin/$@ $<

seq-run: seq bin/seq
	./bin/seq 1200 500

par1: par1.cpp
	mpicxx -O3 $< -o bin/$@

par1-run: par1 bin/par1
	# --oversubscribe --use-hwthread-cpus
	mpirun -np 6 ./bin/par1 1200 500

par2: par2.cpp
	mpicxx -O3 $< -o bin/$@

par2-run: par2 bin/par2
	# --oversubscribe --use-hwthread-cpus
	mpirun -np 6 ./bin/par2 1200 500

par2g: par2.cpp
	mpicxx -O3 -DSF $< -o bin/$@

par2-gif: par2g bin/par2g
	# --oversubscribe --use-hwthread-cpus
	mpirun -np 6 ./bin/par2 1200 500
	$(MAKE) gif

omp: omp.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

gif:
	@ffmpeg -y -framerate 30 -i frames/frame_%04d.pgm life.gif

clean:
	rm -f $(TARGETS) *.csv *.png life.gif
	rm -rf frames
