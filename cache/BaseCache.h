#ifndef BASECACHE_H
#define	BASECACHE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
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
        unsigned int    totalPrefetchedLines;
        unsigned int    totalPrefetchedLinesHit;
        unsigned int    totalPrefetchedUnusedLines;
    
    public:
        BaseCache(char*,unsigned int,unsigned int,unsigned int);
        ~BaseCache();
        virtual int find(unsigned int,unsigned int)=0;
        virtual void promotion(unsigned int,unsigned int, bool)=0;
        virtual unsigned int victimize(unsigned int)=0;
        virtual void eviction(unsigned int,unsigned int)=0;
        virtual void insertion(unsigned int,unsigned int,unsigned int, bool)=0;
        virtual int update(unsigned int, bool)=0;
        
        void heart_beat_stats();
        void final_stats(int);
        
    private:
        unsigned int log2(unsigned int);
};

#endif	/* BASECACHE_H */



