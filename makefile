CXX=g++
MAIN = sim.cpp
PREFETCHER = prefetcher
CACHE = cache
COMMON = common
CXXFLAGS = -std=c++11 -o2 -Wno-write-strings
DEPS = $(COMMON)/util.o $(PREFETCHER)/wssc.o $(PREFETCHER)/SMSPrefetcher.o $(CACHE)/BaseCache.o $(CACHE)/LRUCache.o $(CACHE)/DRRIPCache.o

all: release debug gdb

.PHONY: gdb
gdb: CXXFLAGS += -g
gdb: EXE = sim_gdb
gdb: $(MAIN)
	@cd $(COMMON) && make gdb
	@cd $(PREFETCHER) && make gdb
	@cd $(CACHE) && make gdb
	$(CXX) $(CXXFLAGS) -o sim_gdb $(MAIN) $(DEPS) -lz

.PHONY: debug
debug: CXXFLAGS += -g -DDEBUG
debug: EXE = sim_debug
debug: $(MAIN)
	@cd $(COMMON) && make debug
	@cd $(PREFETCHER) && make debug
	@cd $(CACHE) && make debug
	$(CXX) $(CXXFLAGS) -o sim_debug $(MAIN) $(DEPS) -lz

.PHONY: release
release: $(MAIN)
	@cd $(COMMON) && make release
	@cd $(PREFETCHER) && make release
	@cd $(CACHE) && make release
	$(CXX) $(CXXFLAGS) -o sim $(MAIN) $(DEPS) -lz

.PHONY: clean
clean:
	@cd $(COMMON) && make clean
	@cd $(PREFETCHER) && make clean
	@cd $(CACHE) && make clean
	@rm -f sim sim_debug sim_gdb
	@rm -f analysis
