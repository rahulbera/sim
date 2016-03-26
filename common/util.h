#ifndef UTIL_H
#define UTIL_H

unsigned int log2(unsigned int);

class Monitor
{
    unsigned int c;
    unsigned int max_count;
    
    public:
    	Monitor();
        Monitor(unsigned n);
        ~Monitor();
        void set(unsigned int);
        void inc();
        void dec();
        unsigned int value();
        unsigned int max();
};

#endif /* UTIL_H */

