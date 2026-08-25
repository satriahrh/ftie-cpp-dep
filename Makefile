experiment-build-a:
	g++ -o bin/exp-a.o -std=c++17 ftie/*.h ftie/*.cpp experiment/tools.h experiment/tools.cpp experiment/a.cpp `libpng-config --ldflags`
experiment-build-b:
	g++ -o bin/exp-b.o -std=c++17 ftie/*.h ftie/*.cpp experiment/tools.h experiment/tools.cpp experiment/b.cpp `libpng-config --ldflags`
experiment-build-c:
	g++ -o bin/exp-c.o -std=c++17 ftie/*.h ftie/*.cpp experiment/tools.h experiment/tools.cpp experiment/c.cpp `libpng-config --ldflags`
experiment-build-d:
	g++ -o bin/exp-d.o -std=c++17 ftie/*.h ftie/*.cpp experiment/tools.h experiment/tools.cpp experiment/d.cpp `libpng-config --ldflags`
experiment-build-e:
	g++ -o bin/exp-e.o -std=c++17 ftie/*.h ftie/*.cpp experiment/tools.h experiment/tools.cpp experiment/e.cpp `libpng-config --ldflags`
experiment-build-f:
	g++ -o bin/exp-f.o -std=c++17 ftie/*.h ftie/*.cpp experiment/tools.h experiment/tools.cpp experiment/f.cpp `libpng-config --ldflags`
experiment-build-all:
	g++ -o bin/exp-a.o -std=c++17 ftie/*.h ftie/*.cpp experiment/tools.h experiment/tools.cpp experiment/a.cpp `libpng-config --ldflags`
	g++ -o bin/exp-b.o -std=c++17 ftie/*.h ftie/*.cpp experiment/tools.h experiment/tools.cpp experiment/b.cpp `libpng-config --ldflags`
	g++ -o bin/exp-c.o -std=c++17 ftie/*.h ftie/*.cpp experiment/tools.h experiment/tools.cpp experiment/c.cpp `libpng-config --ldflags`
	g++ -o bin/exp-d.o -std=c++17 ftie/*.h ftie/*.cpp experiment/tools.h experiment/tools.cpp experiment/d.cpp `libpng-config --ldflags`
	g++ -o bin/exp-e.o -std=c++17 ftie/*.h ftie/*.cpp experiment/tools.h experiment/tools.cpp experiment/e.cpp `libpng-config --ldflags`
	g++ -o bin/exp-f.o -std=c++17 ftie/*.h ftie/*.cpp experiment/tools.h experiment/tools.cpp experiment/f.cpp `libpng-config --ldflags`
experiment-run-all:
	./bin/exp-a.o
	./bin/exp-b.o
	./bin/exp-c.o
	./bin/exp-d.o
	./bin/exp-e.o
	./bin/exp-f.o
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
	./bin/exp-f.o
