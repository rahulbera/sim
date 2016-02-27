CXX = g++
CORE = BaseCache.o LRUCache.o
CXXFLAGS = -o2

all: release debug

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
