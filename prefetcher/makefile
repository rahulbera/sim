CXX = g++
CORE = wssc.o SMSPrefetcher.o
CXXFLAGS = -o2 --std=c++11

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
	@echo "Cleaning prefetcher directory..."
	@rm -f *.o
