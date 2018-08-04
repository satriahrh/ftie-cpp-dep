experiment-build-a:
	g++ -o bin/exp-a -std=c++17 ftie/*.h ftie/*.cpp experiment/tools.h experiment/tools.cpp experiment/a.cpp `libpng-config --ldflags`
experiment-build-b:
	g++ -o bin/exp-b -std=c++17 ftie/*.h ftie/*.cpp experiment/tools.h experiment/tools.cpp experiment/b.cpp `libpng-config --ldflags`
experiment-build-c:
	g++ -o bin/exp-c -std=c++17 ftie/*.h ftie/*.cpp experiment/tools.h experiment/tools.cpp experiment/c.cpp `libpng-config --ldflags`
experiment-build-d:
	g++ -o bin/exp-d -std=c++17 ftie/*.h ftie/*.cpp experiment/tools.h experiment/tools.cpp experiment/d.cpp `libpng-config --ldflags`
experiment-build-e:
	g++ -o bin/exp-e -std=c++17 ftie/*.h ftie/*.cpp experiment/tools.h experiment/tools.cpp experiment/e.cpp `libpng-config --ldflags`
experiment-build-f:
	g++ -o bin/exp-f -std=c++17 ftie/*.h ftie/*.cpp experiment/tools.h experiment/tools.cpp experiment/f.cpp `libpng-config --ldflags`
experiment-build-all:
	g++ -o bin/exp-a -std=c++17 ftie/*.h ftie/*.cpp experiment/tools.h experiment/tools.cpp experiment/a.cpp `libpng-config --ldflags`
	g++ -o bin/exp-b -std=c++17 ftie/*.h ftie/*.cpp experiment/tools.h experiment/tools.cpp experiment/b.cpp `libpng-config --ldflags`
	g++ -o bin/exp-c -std=c++17 ftie/*.h ftie/*.cpp experiment/tools.h experiment/tools.cpp experiment/c.cpp `libpng-config --ldflags`
	g++ -o bin/exp-d -std=c++17 ftie/*.h ftie/*.cpp experiment/tools.h experiment/tools.cpp experiment/d.cpp `libpng-config --ldflags`
	g++ -o bin/exp-e -std=c++17 ftie/*.h ftie/*.cpp experiment/tools.h experiment/tools.cpp experiment/e.cpp `libpng-config --ldflags`
	g++ -o bin/exp-f -std=c++17 ftie/*.h ftie/*.cpp experiment/tools.h experiment/tools.cpp experiment/f.cpp `libpng-config --ldflags`
experiment-run-all:
	./bin/exp-a
	./bin/exp-b
	./bin/exp-c
	./bin/exp-d
	./bin/exp-e
	./bin/exp-f
experiment-run-a:
	./bin/exp-a
experiment-run-b:
	./bin/exp-b
experiment-run-c:
	./bin/exp-c
experiment-run-d:
	./bin/exp-d
experiment-run-e:
	./bin/exp-e
experiment-run-f:
	./bin/exp-f
