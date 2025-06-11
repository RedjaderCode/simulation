
files = src\main.cpp
obj   = main.o

include	    = -IC:\code\redjader\simulation\include
linker      = -lgdi32 -ld2d1

flag = -w -fstack-usage -DUNICODE -ggdb

$(obj): $(files)
	g++ -c $(files) $(linker) $(include) $(flag)
	g++ $(obj) $(linker) $(include) $(flag) -o sim
	rm *.o *.su

S: $(files)
	g++ -S $(files) $(linker) $(include) $(flag)

clean:
	rm *.o *.su
