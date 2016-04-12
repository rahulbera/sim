#ifndef DRRIPCACHE_H
#define	DRRIPCACHE_H

#include "BaseCache.h"
#include "../common/util.h"
#include "../prefetcher/wssc.h"

typedef unsigned int uint;

class DRRIPCache : public BaseCache
{
    public:
        bool lite;
        wssc* wssc_map;
        unsigned long int wssc_interval;

        /* own additional variables */
        uint **rrpv;
        uint maxRRPV;
        uint throttleCounter;
        uint throttleParam;
        Monitor psel;


        /* Prefetcher related counters */
        unsigned int totalPrefetchedLines;
        unsigned int totalPrefetchedUnusedLines;
        unsigned int stat_total_early_prefetch;
        unsigned int perfect_prefetch;
        unsigned int miss_prediction;
        
    public:
        DRRIPCache();
        ~DRRIPCache();

        /* Base class functions */
        void init(char*,char*,uint,uint,uint,uint,uint,uint,bool);
        int find(uint,uint);
        void promotion(uint,uint,bool);
        uint victimize(uint);
        void eviction(uint,uint);
        void insertion(uint,uint,uint,uint,bool);
        void insertion(uint,uint,uint,uint,bool,bool);
        int update(uint,uint,bool);
        void heart_beat_stats(int);
        void final_stats(int);
		void release();

        /* Own fucntions */
        void link_wssc(wssc*);
        void wrap_up();

};

#endif	/* DRRIPCACHE_H */

