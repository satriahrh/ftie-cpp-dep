# experiment f (corpus): manifest in, one CSV row per file out
MANIFEST ?= experiment/corpus_manifest.csv
OUT ?= experiment/results/f.csv
# extra compiler flags, e.g. on macOS/Homebrew:
#   make EXTRA_FLAGS="-I/opt/homebrew/include -L/opt/homebrew/lib `libpng-config --cflags`" ...
EXTRA_FLAGS ?=

experiment-gen-params:
	g++ -o bin/exp-gen-params.o -std=c++17 $(EXTRA_FLAGS) ftie/prime.cpp experiment/gen_params.cpp -lgmpxx -lgmp
	./bin/exp-gen-params.o
experiment-build-a:
	g++ -o bin/exp-a.o -std=c++17 $(EXTRA_FLAGS) ftie/*.cpp experiment/tools.cpp experiment/a.cpp `libpng-config --ldflags` -lgmpxx -lgmp
experiment-build-b:
	g++ -o bin/exp-b.o -std=c++17 $(EXTRA_FLAGS) ftie/*.cpp experiment/tools.cpp experiment/b.cpp `libpng-config --ldflags` -lgmpxx -lgmp
experiment-build-c:
	g++ -o bin/exp-c.o -std=c++17 $(EXTRA_FLAGS) ftie/*.cpp experiment/tools.cpp experiment/c.cpp `libpng-config --ldflags` -lgmpxx -lgmp
experiment-build-d:
	g++ -o bin/exp-d.o -std=c++17 $(EXTRA_FLAGS) ftie/*.cpp experiment/tools.cpp experiment/d.cpp `libpng-config --ldflags` -lgmpxx -lgmp
experiment-build-e:
	g++ -o bin/exp-e.o -std=c++17 $(EXTRA_FLAGS) ftie/*.cpp experiment/tools.cpp experiment/e.cpp `libpng-config --ldflags` -lgmpxx -lgmp
experiment-build-f:
	g++ -o bin/exp-f.o -std=c++17 $(EXTRA_FLAGS) ftie/*.cpp experiment/tools.cpp experiment/manifest.cpp experiment/sha256.cpp experiment/f.cpp `libpng-config --ldflags` -lgmpxx -lgmp
experiment-build-all:
	g++ -o bin/exp-a.o -std=c++17 $(EXTRA_FLAGS) ftie/*.cpp experiment/tools.cpp experiment/a.cpp `libpng-config --ldflags` -lgmpxx -lgmp
	g++ -o bin/exp-b.o -std=c++17 $(EXTRA_FLAGS) ftie/*.cpp experiment/tools.cpp experiment/b.cpp `libpng-config --ldflags` -lgmpxx -lgmp
	g++ -o bin/exp-c.o -std=c++17 $(EXTRA_FLAGS) ftie/*.cpp experiment/tools.cpp experiment/c.cpp `libpng-config --ldflags` -lgmpxx -lgmp
	g++ -o bin/exp-d.o -std=c++17 $(EXTRA_FLAGS) ftie/*.cpp experiment/tools.cpp experiment/d.cpp `libpng-config --ldflags` -lgmpxx -lgmp
	g++ -o bin/exp-e.o -std=c++17 $(EXTRA_FLAGS) ftie/*.cpp experiment/tools.cpp experiment/e.cpp `libpng-config --ldflags` -lgmpxx -lgmp
	g++ -o bin/exp-f.o -std=c++17 $(EXTRA_FLAGS) ftie/*.cpp experiment/tools.cpp experiment/manifest.cpp experiment/sha256.cpp experiment/f.cpp `libpng-config --ldflags` -lgmpxx -lgmp
experiment-run-all:
	./bin/exp-a.o
	./bin/exp-b.o
	./bin/exp-c.o
	./bin/exp-d.o
	./bin/exp-e.o
	./bin/exp-f.o $(MANIFEST) $(OUT)
experiment-run-a:
	./bin/exp-a.o
experiment-run-b:
	./bin/exp-b.o
experiment-run-c:
	./bin/exp-c.o
experiment-run-d:
	./bin/exp-d.o
experiment-run-e:
	./bin/exp-e.o
experiment-run-f:
	./bin/exp-f.o $(MANIFEST) $(OUT)

