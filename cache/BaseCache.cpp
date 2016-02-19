#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "BaseCache.h"

CacheLine::CacheLine(unsigned int b)
{
    size = b;
    tag=0;
    age=0;
    reuseCount = 0;
    valid=false;
}

unsigned int BaseCache::log2(unsigned int n)
{
    if(n==1) return 0;
    else return (1+log2(n/2));
}

BaseCache::BaseCache(char *s, unsigned int a,unsigned int b,unsigned int c)
{
    strcpy(name, s);
    fprintf(stderr, "Blah\n");
    associativity=a;
    noOfSets = c/(a*b);
    setsInPowerOfTwo = log2(noOfSets);
    offset = log2(b);   
    cache = (CacheLine **)malloc(noOfSets*sizeof(CacheLine*));
    for(unsigned int i=0;i<noOfSets;++i)
        cache[i] = (CacheLine*)malloc(associativity*sizeof(CacheLine));
    for(unsigned int i=0;i<noOfSets;++i)
    {
        for(unsigned int j=0;j<associativity;++j)
            cache[i][j] = CacheLine(b);
    }
    
    totalAccess = hit = miss = 0;
    for(unsigned int i=0;i<16;++i) reuseCountBucket[i] = 0;
}

BaseCache::~BaseCache()
{
    free(cache);
}

void BaseCache::heart_beat_stats(int verbose)
{

}

void BaseCache::final_stats(int verbose)
{
    fprintf(stdout, "[ Cache.%s ]\n", name);
    fprintf(stdout, "TotalAccesses =  %d\n", totalAccess);
    fprintf(stdout, "Hits =  %d\n", hit);
    fprintf(stdout, "Misses =  %d [%0.2f]\n", miss, (float)miss/totalAccess*100);
    if(verbose > 0)
    {
        fprintf(stdout, "Reuse count stats:\n");
        unsigned int total = 0;
        for(int i=0;i<16;++i) total+=reuseCountBucket[i];
        for(int i=0;i<16;++i)
            fprintf(stdout, "%d: %0.2f\n", i, (float)reuseCountBucket[i]/total*100);
    }
    if(verbose > 1)
    {
        for(int i=0;i<noOfSets;++i)
        {
            fprintf(stdout, "::::::::::::::: SET %d ::::::::::::::::\n", i);
            for(unsigned int j=0;j<associativity;++j)
                fprintf(stdout, "%d\t%d\t%d\n", cache[i][j].tag, cache[i][j].valid, cache[i][j].age);
            fprintf(stdout, "\n");
        }
    }
}