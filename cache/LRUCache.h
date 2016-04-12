#ifndef LRUCACHE_H
#define	LRUCACHE_H

#include "BaseCache.h"
#include <unordered_map>
#include "../prefetcher/wssc.h"


typedef struct stat
{
    unsigned int counter1, counter2, counter3;
}stat_t;

typedef struct early_prefetch_entry
{
    unsigned int pc;
    unsigned int count;
}ep_t;

class wssc;

class LRUCache : public BaseCache
{
    public:
        bool lite;
        wssc* wssc_map;
        bool wssc_on;
        unsigned long int wssc_interval;
        /* Stores prefetch lines which are evicted from cache 
         * without getting demand access in <LINE,ep_t> format
         */
        std::unordered_map<unsigned int, ep_t*> early_prefetch;
        /* Stores total misses faced by a PC 
         * and number of early prefetch
         */
        std::unordered_map<unsigned int,stat_t*> demand_miss_map;
        /* Stores total prefetch generated by a PC
         * number of early prefect prefetches.
         */
        std::unordered_map<unsigned int, stat_t*> pc_prefetch_map;
        
        /* Prefetcher related counters */
        unsigned int totalPrefetchedLines;
        unsigned int totalPrefetchedUnusedLines;
        unsigned int stat_total_early_prefetch;
        unsigned int perfect_prefetch;
        unsigned int miss_prediction;
        
    public:
        LRUCache();
        ~LRUCache();
        
        /*Base class functions */
        void init(char*,char*,unsigned int,unsigned int,unsigned int,bool);
        int find(unsigned int,unsigned int);
        int find(unsigned int);
        void promotion(unsigned int,unsigned int, bool);
        unsigned int victimize(unsigned int);
        void eviction(unsigned int,unsigned int);
        void insertion(unsigned int,unsigned int,unsigned int,unsigned int, bool);
        int update(unsigned int, unsigned int, bool);
        void heart_beat_stats(int);
        void final_stats(int);
		void release();

        /* Own functions */
        void link_wssc(wssc*, bool);
        void insert_early_prefetch_map(unsigned int, unsigned int);
        void update_early_prefetch_map(unsigned int, unsigned int);
        void insert_pc_prefetch_map(unsigned int);
        void update_pc_prefetch_map(unsigned int);
        void wrap_up();

};

#endif	/* LRUCACHE_H */



