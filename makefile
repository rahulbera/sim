CC=g++
MAIN = sim.cpp
PREFETCHER = prefetcher
CACHE = cache


all: sim

sim: $(MAIN)
	@cd $(PREFETCHER) && make
	@cd $(CACHE) && make
	$(CC) -o2 -Wno-write-strings -o sim $(MAIN) $(PREFETCHER)/StreamPrefetcher.o $(PREFETCHER)/SMSPrefetcher.o $(CACHE)/BaseCache.o $(CACHE)/LRUCache.o -lz	

clean:
	@cd $(PREFETCHER) && make clean
	@cd $(CACHE) && make clean
	@rm -f sim
