CC = g++
CORE = StreamPrefetcher.o SMSPrefetcher.o

all: $(CORE)

%.o: %.cpp 
	@echo "Making $@..."
	@$(CC) -c -o2 -o $@ $<

clean:
	@echo "Cleaning prefetcher directory..."
	@rm -f *.o