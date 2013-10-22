
prg := daytimeclient daytimeclient2 daytimeserver \
	daytimeserver2 daytimeserver3 daytimeserver4 \
	daytimeserver5

CFLAGS := -g -Wall -Wno-unused-function

all : $(prg)

daytimeserver3 : daytimeserver3.c
	gcc -o $@ $(CFLAGS) $< -lpthread

clean:
	@rm -fv $(prg)
