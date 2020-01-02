CC = gcc
CFLAGS = -Wall -std=gnu99
LD = gcc
LDFLAGS = -g -std=gnu99
LDLIBS = 

DEBUGFLAG = 0

SRCS = sgp30-cli.c gp30.c
OBJS = sgp30-cli.o sgp30.o
TARGETS = sgp30-cli debug

debug: CFLAGS += -DDEBUG -g

all: ${TARGETS}

sgp30-cli: $(OBJS)
	$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS)

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
