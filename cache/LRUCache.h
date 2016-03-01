#ifndef LRUCACHE_H
#define	LRUCACHE_H

#include "BaseCache.h"

class LRUCache : public BaseCache
{
    public:
        
        
    public:
        LRUCache(char*,unsigned int,unsigned int,unsigned int);
        ~LRUCache();
        int find(unsigned int,unsigned int);
        int find(unsigned int);
        void promotion(unsigned int,unsigned int, bool);
        unsigned int victimize(unsigned int);
        void eviction(unsigned int,unsigned int);
        void insertion(unsigned int,unsigned int,unsigned int,unsigned int, bool);
        int update(unsigned int, unsigned int, bool);

};

#endif	/* LRUCACHE_H */



