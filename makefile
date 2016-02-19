CC=g++
MAIN = sim.cpp
PREFETCHER = prefetcher
CACHE = cache


all: stream

stream: $(MAIN)
	@cd $(PREFETCHER) && make
	@cd $(CACHE) && make
	$(CC) -o2 -o sim_stream $(MAIN) $(PREFETCHER)/stream_prefetcher.o -lz	

clean:
	@cd $(PREFETCHER) && make clean
	@cd $(CACHE) && make clean
	@rm -f sim_stream