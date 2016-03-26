#include "util.h"


unsigned int log2(unsigned int n)
{
	if(n==1) return 0;
    else return (1+log2(n/2));
}

Monitor::Monitor()
{

}

Monitor::Monitor(unsigned int n)
{
	max_count = (1 << n) - 1;
	c = 1 << (n-1);
}

Monitor::~Monitor()
{

}

void Monitor::set(unsigned int n)
{
	max_count = (1 << n) - 1;
	c = 1 << (n-1);
}

void Monitor::inc()
{
	if(c < max_count) c++;
}

void Monitor::dec()
{
	if(c > 0) c--;
}

unsigned int Monitor::value()
{
	return c;
}

unsigned int Monitor::max()
{
	return max_count;
}
