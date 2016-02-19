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
        void promotion(unsigned int,unsigned int);
        unsigned int victimize(unsigned int);
        void eviction(unsigned int,unsigned int);
        void insertion(unsigned int,unsigned int,unsigned int);
        int update(unsigned int);

};

#endif	/* LRUCACHE_H */



