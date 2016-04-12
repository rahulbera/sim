CXX = g++
CORE = BaseCache.o LRUCache.o DRRIPCache.o
CXXFLAGS = -std=c++11 -o2

all: release debug gdb

.PHONY: gdb
gdb: CXXFLAGS += -g
gdb: $(CORE)

.PHONY: debug
debug: CXXFLAGS += -g -DDEBUG
debug: $(CORE)

.PHONY: release
release: $(CORE)

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<

clean:
	@echo "Cleaning cache directory..."
	@rm -f *.o
