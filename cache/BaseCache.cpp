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
    stat_heart_beat_total_access = stat_heart_beat_hit = stat_heart_beat_miss = 0;
    totalPrefetchedLines = totalPrefetchedUnusedLines = 0;
    for(unsigned int i=0;i<16;++i) reuseCountBucket[i] = 0;
}

BaseCache::~BaseCache()
{
    free(cache);
}

void BaseCache::heart_beat_stats()
{
    fprintf(stderr, "%s.Total:%d %s.Miss: %d [%0.2f] ", name, stat_heart_beat_total_access, name, stat_heart_beat_miss, (float)stat_heart_beat_miss/stat_heart_beat_total_access*100);
    stat_heart_beat_total_access = stat_heart_beat_hit = stat_heart_beat_miss = 0;
}

void BaseCache::final_stats(int verbose)
{
    fprintf(stdout, "[ Cache.%s ]\n", name);
    fprintf(stdout, "TotalAccesses =  %d\n", totalAccess);
    fprintf(stdout, "Hits =  %d\n", hit);
    fprintf(stdout, "Misses =  %d [%0.2f]\n", miss, (float)miss/totalAccess*100);
    fprintf(stdout, "Prefetched = %d\n", totalPrefetchedLines);
    fprintf(stdout, "PrefetchedUnused = %d\n", totalPrefetchedUnusedLines);
    if(verbose > 0)
    {
        std::unordered_map<unsigned int,stat_t*>::iterator it;
        for(it=demand_miss_map.begin();it!=demand_miss_map.end();++it)
            fprintf(stdout, "%*x %*d %*d\n", 7, it->first, 6, it->second->total_demand_miss, 6, it->second->total_early_prefetch);
    }
    if(verbose > 1)
    {
        fprintf(stdout, "Reuse count stats:\n");
        unsigned int total = 0;
        for(int i=0;i<16;++i) total+=reuseCountBucket[i];
        for(int i=0;i<16;++i)
            fprintf(stdout, "%d: %0.2f\n", i, (float)reuseCountBucket[i]/total*100);
    }
    if(verbose > 2)
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