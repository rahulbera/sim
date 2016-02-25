CC = g++
CORE = BaseCache.o LRUCache.o
CCFLAGS = -g -o2

all: $(CORE)

%.o: %.cpp
	$(CC) -c $(CCFLAGS) -o $@ $<

clean:
	@echo "Cleaning cache directory..."
	@rm -f *.o
