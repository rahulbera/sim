#ifndef BASECACHE_H
#define	BASECACHE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include "../common/util.h"
#define ASSERT(cond,msg) do{if(!(cond)){fprintf(stderr,msg);exit(1);}}while(0)

class CacheLine
{
    public:
        unsigned int    tag;
        unsigned int    reuseCount;
        unsigned int    pc;
        unsigned int    signature;
        int             age;
        bool            valid;
        bool            pref;
        bool            pref_hit;
        bool            pref_pred;

    public:
	CacheLine();
};

class BaseCache
{
    public:
        char            name[100];
        char            type[20];
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
    
    public:
        BaseCache();
        ~BaseCache();
        
        virtual void init(char*,char*,unsigned int,unsigned int,unsigned int);
        virtual int find(unsigned int,unsigned int)=0;
        virtual void promotion(unsigned int,unsigned int, bool)=0;
        virtual unsigned int victimize(unsigned int)=0;
        virtual void eviction(unsigned int,unsigned int)=0;
        virtual void insertion(unsigned int,unsigned int,unsigned int,unsigned int, bool)=0;
        virtual int update(unsigned int, unsigned int, bool)=0;
		virtual void release();
        
        virtual void heart_beat_stats(int);
        virtual void final_stats(int);
};

#endif	/* BASECACHE_H */



