CC = gcc
CFLAGS = -Wall -std=gnu99
LD = gcc
LDFLAGS = -g -std=gnu99
LDLIBS = 

DEBUGFLAG = 0

SRCS = sgp30.c bcm2835.c
OBJS = sgp30.o bcm2835.o
TARGETS = sgp30 debug

all: ${TARGETS}

sgp30: $(OBJS)
	$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS)

debug: CFLAGS += -DDEBUG -g
debug: $(OBJS)
	$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS)

%.o: %.c 
	$(CC) $(CFLAGS) -c $< 

%.d: %.c
	gcc -MM -MF $@ $<

-include $(SRCS:.c=.d)

.PHONY: clean
clean:
	rm -f *~ *.d *.o $(TARGETS) 
