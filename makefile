out.exe: main.cpp
	g++ main.cpp -g3 -o out.exe -Llib/ -lSDL2
	./out.exe
