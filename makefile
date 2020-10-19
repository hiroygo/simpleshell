simpleshell: main.o Job.o Pipe.o
	g++-9 -std=c++17 -Wall -Wextra -o simpleshell $^
%.o: %.cpp
	g++-9 -std=c++17 -Wall -Wextra -c -o $@ $<