#ifndef BASECACHE_H
#define	BASECACHE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unordered_set>
#include <unordered_map>
#define MAX 100

class CacheLine
{
    public:
        unsigned int    size;
        unsigned int    tag;
        unsigned int    age;
        unsigned int    reuseCount;
        bool            state; /* 0 signifies it is installed as demand, 1 as prefetched */
        bool            valid;

    public:
	CacheLine(unsigned int);
};

typedef struct stat
{
    unsigned int total_demand_miss;
    unsigned int total_early_prefetch;
}stat_t;

class BaseCache
{
    public:
        char            name[MAX];
        unsigned int    associativity;
        unsigned int    noOfSets;
        unsigned int    setsInPowerOfTwo;
        unsigned int    offset;
        CacheLine       **cache;
        
        unsigned int    reuseCountBucket[16];
        unsigned int    totalAccess;
        unsigned int    hit,miss;
        unsigned int    stat_heart_beat_total_access;
        unsigned int    stat_heart_beat_hit, stat_heart_beat_miss;

        /* Prefetcher related counters */
        unsigned int    totalPrefetchedLines;
        unsigned int    totalPrefetchedUnusedLines;
        std::unordered_set<unsigned int> unused_prefetch_set;
        std::unordered_map<unsigned int,stat_t*> demand_miss_map;
    
    public:
        BaseCache(char*,unsigned int,unsigned int,unsigned int);
        ~BaseCache();
        virtual int find(unsigned int,unsigned int)=0;
        virtual void promotion(unsigned int,unsigned int, bool)=0;
        virtual unsigned int victimize(unsigned int)=0;
        virtual void eviction(unsigned int,unsigned int)=0;
        virtual void insertion(unsigned int,unsigned int,unsigned int,unsigned int, bool)=0;
        virtual int update(unsigned int, unsigned int, bool)=0;
        
        void heart_beat_stats();
        void final_stats(int);
        
    private:
        unsigned int log2(unsigned int);
};

#endif	/* BASECACHE_H */



