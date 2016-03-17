#include "util.h"


unsigned int log2(unsigned int n)
{
	if(n==1) return 0;
    else return (1+log2(n/2));
}


Counter::Counter(unsigned int n)
{
	max_count = (1 << n) - 1;
	c = 1 << (n-1);
}

Counter::~Counter()
{

}

void Counter::inc()
{
	if(c < max_count) c++;
}

void Counter::dec()
{
	if(c > 0) c--;
}

unsigned int Counter::value()
{
	return c;
}

unsigned int Counter::max()
{
	return max_count;
}
