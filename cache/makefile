CC = g++
CORE = BaseCache.o LRUCache.o

all: $(CORE)

%.o: %.cpp
	@echo "Making $@..."
	@$(CC) -c -o $@ $<

clean:
	@echo "Cleaning cache directory..."
	@rm -f *.o