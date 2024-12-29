all: main

main: main.cpp
	g++ -o main main.cpp shaderStuff.cpp player.cpp -lsfml-graphics -lsfml-window -lsfml-system -lGL -lGLEW -lGLU