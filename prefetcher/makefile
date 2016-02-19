CC = g++
CORE = stream_prefetcher.o stride_prefetcher.o stride_cache_prefetcher.o pc_cs_prefetcher.o

all: $(CORE)

%.o: %.cpp 
	@echo "Making $@..."
	@$(CC) -c -o $@ $<

clean:
	@echo "Cleaning prefetcher directory..."
	@rm -f *.o