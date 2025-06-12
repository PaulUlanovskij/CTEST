main: ctest.c .NECO 
	gcc -ggdb -o ./build/ctest ./ctest.c ./.NECO/*.c 

run: main
	./build/ctest
