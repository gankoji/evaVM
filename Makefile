all: clean
	clang++ -std=c++17 -Wall -ggdb3 -I. ./EvaVM.cpp -o ./eva-vm
	./eva-vm

clean:
	rm ./eva-vm