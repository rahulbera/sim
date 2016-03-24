#ifndef UTIL_H
#define UTIL_H

unsigned int log2(unsigned int);

class Counter
{
    unsigned int c;
    unsigned int max_count;
    
    public:
        Counter(unsigned n);
        ~Counter();
        void inc();
        void dec();
        unsigned int value();
        unsigned int max();
};

#endif /* UTIL_H */

