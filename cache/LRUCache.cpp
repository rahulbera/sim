#include "LRUCache.h"

LRUCache::LRUCache(char *s, unsigned int a, unsigned int b, unsigned int c):BaseCache(s,a,b,c)
{
    
}

LRUCache::~LRUCache()
{
    
}

int LRUCache::find(unsigned int setIndex, unsigned int tag)
{
    for(unsigned int i=0;i<associativity;++i)
    {
        if(cache[setIndex][i].tag==tag && cache[setIndex][i].valid)
            return i;
    }
    return -1;
}

int LRUCache::find(unsigned int addr)
{
    unsigned int temp = addr >> offset;
    unsigned int setIndex = temp & (noOfSets-1);
    unsigned int tag = temp >> setsInPowerOfTwo;

    return find(setIndex,tag);
}

void LRUCache::promotion(unsigned int setIndex, unsigned int wayIndex, bool isPrefetched)
{
    for(unsigned int i=0;i<associativity;++i)
    {
        if(i!=wayIndex && cache[setIndex][i].valid && cache[setIndex][i].age<cache[setIndex][wayIndex].age)
            cache[setIndex][i].age++;
    }
    cache[setIndex][wayIndex].age = 0;
    cache[setIndex][wayIndex].reuseCount++;
    //cache[setIndex][wayIndex].isPrefetched = isPrefetched;
}

unsigned int LRUCache::victimize(unsigned int setIndex)
{
    for(unsigned int i=0;i<associativity;++i)
    {
        if(cache[setIndex][i].age == (associativity-1) || !cache[setIndex][i].valid)
            return i;
    }
}

void LRUCache::eviction(unsigned int setIndex, unsigned int wayIndex)
{
    if(cache[setIndex][wayIndex].reuseCount>=15)
        reuseCountBucket[15]++;
    else
        reuseCountBucket[cache[setIndex][wayIndex].reuseCount]++;
    if(cache[setIndex][wayIndex].state)
    {
        if(cache[setIndex][wayIndex].reuseCount == 0)
            totalPrefetchedUnusedLines++;
        else
            totalPrefetchedLinesHit += cache[setIndex][wayIndex].reuseCount;
    }
    
    cache[setIndex][wayIndex].reuseCount = 0;
    cache[setIndex][wayIndex].valid = false;
}

void LRUCache::insertion(unsigned int setIndex, unsigned int wayIndex, unsigned int tag, bool isPrefetched)
{
    cache[setIndex][wayIndex].tag = tag;
    cache[setIndex][wayIndex].age = 0;
    cache[setIndex][wayIndex].valid = true;
    cache[setIndex][wayIndex].state = isPrefetched;
    if(isPrefetched) totalPrefetchedLines++;
    for(unsigned int i=0;i<associativity;++i)
    {
        if(i!=wayIndex && cache[setIndex][i].valid)
            cache[setIndex][i].age++;
    }
}

int LRUCache::update(unsigned int addr, bool isPrefetched)
{
    unsigned int temp = addr >> offset;
    unsigned int setIndex = temp & (noOfSets-1);
    unsigned int tag = temp >> setsInPowerOfTwo;

    if(!isPrefetched) totalAccess++;
    int wayIndex = find(setIndex,tag);
    if(wayIndex != -1)
    {
        if(!isPrefetched) hit++;
        if(!isPrefetched) promotion(setIndex,wayIndex, isPrefetched);
        return 1;
    }        
    else
    {
        if(!isPrefetched) miss++;
        unsigned int victimWayIndex = victimize(setIndex);
        if(cache[setIndex][victimWayIndex].valid)
            eviction(setIndex,victimWayIndex);
        insertion(setIndex,victimWayIndex,tag, isPrefetched);
        return -1;
    }
    
}


