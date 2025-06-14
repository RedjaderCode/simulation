
files = src\main.cpp
obj   = main.o

include	    = -IC:\code\redjader\simulation\include
linker      = -lgdi32 -ld2d1

flag = -w -fstack-usage -DUNICODE -g -std=c++20 -pthread -fno-inline -fno-omit-frame-pointer -O0 -Wall

debug = -O0 -Wall

$(obj): $(files)
	g++ -c $(files) $(linker) $(include) $(flag)
	g++ $(obj) $(linker) $(include) $(flag) -o sim
	rm *.o *.su

S: $(files)
	g++ -S $(files) $(linker) $(include) $(flag) -fverbose-asm

clean:
	rm *.o *.su
