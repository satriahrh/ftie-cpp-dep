build:
	g++ ftie/*.h ftie/*.cpp gui/tampilan.h gui/tampilan.cpp main-gui.cpp -o bin/main-gui.o -std=c++17 `pkg-config gtkmm-3.0 libpng --cflags --libs`
start:
	./bin/main-gui.o
build-start:
	make build && make start
