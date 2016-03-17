CXX=g++
MAIN = sim.cpp
PREFETCHER = prefetcher
CACHE = cache
CXXFLAGS = -std=c++11 -o2 -Wall
DEPS = $(PREFETCHER)/wssc.o $(PREFETCHER)/SMSPrefetcher.o $(CACHE)/util.o $(CACHE)/BaseCache.o $(CACHE)/LRUCache.o

all: release debug

.PHONY: debug
debug: CXXFLAGS += -g -DDEBUG
debug: EXE = sim_debug
debug: $(MAIN)
	@cd $(PREFETCHER) && make debug
	@cd $(CACHE) && make debug
	$(CXX) $(CXXFLAGS) -o sim_debug $(MAIN) $(DEPS) -lz

.PHONY: release
release: $(MAIN)
	@cd $(PREFETCHER) && make release
	@cd $(CACHE) && make release
	$(CXX) $(CXXFLAGS) -o sim $(MAIN) $(DEPS) -lz

.PHONY: clean
clean:
	@cd $(PREFETCHER) && make clean
	@cd $(CACHE) && make clean
	@rm -f sim sim_debug
	@rm -f analysis
