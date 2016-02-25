CC = g++
CORE = StreamPrefetcher.o SMSPrefetcher.o
CCFLAGS = -g -o2

all: $(CORE)

%.o: %.cpp 
	$(CC) -c $(CCFLAGS) -o $@ $<

clean:
	@echo "Cleaning prefetcher directory..."
	@rm -f *.o
