CXX=g++
MAIN = sim.cpp
PREFETCHER = prefetcher
CACHE = cache
CXXFLAGS = -o2 -Wall

all: release debug

.PHONY: debug
debug: CXXFLAGS += -g -DDEBUG
debug: $(MAIN)
	@cd $(PREFETCHER) && make debug
	@cd $(CACHE) && make debug
	$(CXX) $(CXXFLAGS) -o sim_debug $(MAIN) $(PREFETCHER)/StreamPrefetcher.o $(PREFETCHER)/SMSPrefetcher.o $(CACHE)/BaseCache.o $(CACHE)/LRUCache.o -lz

.PHONY: release
release: $(MAIN)
	@cd $(PREFETCHER) && make release
	@cd $(CACHE) && make release
	$(CXX) $(CXXFLAGS) -o sim $(MAIN) $(PREFETCHER)/StreamPrefetcher.o $(PREFETCHER)/SMSPrefetcher.o $(CACHE)/BaseCache.o $(CACHE)/LRUCache.o -lz

.PHONY: clean
clean:
	@cd $(PREFETCHER) && make clean
	@cd $(CACHE) && make clean
	@rm -f sim sim_debug
	@rm -f analysis
