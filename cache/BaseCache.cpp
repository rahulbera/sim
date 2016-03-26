#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "BaseCache.h"

CacheLine::CacheLine()
{
    tag = 0;
    pc = 0;
    age = 0;
    reuseCount = 0;
    pref = pref_hit = pref_pred = false;
    valid = false;
}

BaseCache::BaseCache()
{

}

BaseCache::~BaseCache()
{
    free(cache);
}

void BaseCache::init(char *s, char *t, unsigned int a,unsigned int b,unsigned int c)
{
    strcpy(name, s);
    strcpy(type, t);
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
            cache[i][j] = CacheLine();
    }
    
    totalAccess = hit = miss = 0;
    stat_heart_beat_total_access = stat_heart_beat_hit = stat_heart_beat_miss = 0;
    for(unsigned int i=0;i<16;++i) reuseCountBucket[i] = 0;
}


void BaseCache::heart_beat_stats(int verbose)
{
    fprintf(stderr, "%s.Total:%d %s.Miss: %d [%0.2f] ", name, stat_heart_beat_total_access, name, stat_heart_beat_miss, (float)stat_heart_beat_miss/stat_heart_beat_total_access*100);
    stat_heart_beat_total_access = stat_heart_beat_hit = stat_heart_beat_miss = 0;
}

void BaseCache::final_stats(int verbose)
{
    fprintf(stdout, "[ Cache.%s ]\n", name);
    fprintf(stdout, "Type = %s\n", type);
    fprintf(stdout, "TotalAccesses =  %d\n", totalAccess);
    fprintf(stdout, "Hits =  %d\n", hit);
    fprintf(stdout, "Misses =  %d [%0.2f]\n", miss, (float)miss/totalAccess*100);
    if(verbose > 2)
    {
        fprintf(stdout, "Reuse count stats:\n");
        unsigned int total = 0;
        for(int i=0;i<16;++i) total+=reuseCountBucket[i];
        for(int i=0;i<16;++i)
            fprintf(stdout, "%d: %0.2f\n", i, (float)reuseCountBucket[i]/total*100);
    }
    if(verbose > 3)
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