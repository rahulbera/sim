#include <assert.h>
#include "LRUCache.h"

LRUCache::LRUCache()
{

}

LRUCache::~LRUCache()
{

}

void LRUCache::release()
{
    BaseCache::release();
}

void LRUCache::init(char *s, char *t, unsigned int a, unsigned int b, unsigned int c, bool isLite)
{
    BaseCache::init(s,t,a,b,c);
    lite = isLite;
    wssc_map = NULL;
    wssc_interval = 0;
    stat_total_early_prefetch = 0;
    totalPrefetchedLines = totalPrefetchedUnusedLines = 0;
    perfect_prefetch = 0;
    miss_prediction = 0;
}


int LRUCache::find(unsigned int setIndex, unsigned int tag)
{
    for(unsigned int i=0;i<associativity;++i)
    {
        if(cache[setIndex][i].valid && cache[setIndex][i].tag==tag)
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
        if(cache[setIndex][i].valid && cache[setIndex][i].age < cache[setIndex][wayIndex].age)
            cache[setIndex][i].age++;
    }
    cache[setIndex][wayIndex].age = 0;
    
    if(cache[setIndex][wayIndex].pref && !cache[setIndex][wayIndex].pref_hit)
    {
        perfect_prefetch++;
        cache[setIndex][wayIndex].pref_hit = true;
        if(!lite) 
            update_pc_prefetch_map(cache[setIndex][wayIndex].pc);
    }

    cache[setIndex][wayIndex].reuseCount++;
}

unsigned int LRUCache::victimize(unsigned int setIndex)
{
    int index = -1, maxAge = -1;
    for(unsigned int i=0;i<associativity;++i)
    {
        if(!cache[setIndex][i].valid)
        {
            index = i;
            break;
        }
	    else if(cache[setIndex][i].age > maxAge)
        {
            maxAge = cache[setIndex][i].age;
            index = i;
        }
    }
    ASSERT(index!=-1, "Invalid victim selection\n");
    return index;
}

void LRUCache::eviction(unsigned int setIndex, unsigned int wayIndex)
{
    if(cache[setIndex][wayIndex].reuseCount>=15)
        reuseCountBucket[15]++;
    else
        reuseCountBucket[cache[setIndex][wayIndex].reuseCount]++;
    
    if(cache[setIndex][wayIndex].pref && !cache[setIndex][wayIndex].pref_hit)
    {
        totalPrefetchedUnusedLines++;
        if(!lite)
            insert_early_prefetch_map(setIndex, wayIndex);
    }
    
    cache[setIndex][wayIndex].valid = false;
}

void LRUCache::insertion(unsigned int pc, unsigned int setIndex, unsigned int wayIndex, unsigned int tag, bool isPrefetched)
{
    unsigned int lineAddr = ((tag << (setsInPowerOfTwo + offset)) + (setIndex << offset));
    
    if(!isPrefetched && !lite)
        update_early_prefetch_map(pc, lineAddr);

    if(isPrefetched)
    {
        totalPrefetchedLines++;
        if(!lite)
            insert_pc_prefetch_map(pc);
    }

    cache[setIndex][wayIndex].tag = tag;
    cache[setIndex][wayIndex].pc = pc;
    cache[setIndex][wayIndex].valid = true;
    cache[setIndex][wayIndex].reuseCount = 0;
    cache[setIndex][wayIndex].pref = isPrefetched;
    cache[setIndex][wayIndex].pref_hit = false;

    for(unsigned int i=0;i<associativity;++i)
    {
        if(cache[setIndex][i].valid)
            cache[setIndex][i].age++;   
    }
    cache[setIndex][wayIndex].age = 0;
    
}

int LRUCache::update(unsigned int pc, unsigned int addr, bool isPrefetched)
{
    unsigned int temp = addr >> offset;
    unsigned int setIndex = temp & (noOfSets-1);
    unsigned int tag = temp >> setsInPowerOfTwo;

    if(!isPrefetched) 
    {
        totalAccess++;
        stat_heart_beat_total_access++;
        if(wssc_map && wssc_on && (totalAccess % wssc_interval) == 0)
            wssc_map->invalidate();
    }
    int wayIndex = find(setIndex,tag);
    if(wayIndex != -1)
    {
        if(!isPrefetched)
        {
            hit++;
            stat_heart_beat_hit++;
            promotion(setIndex, wayIndex, isPrefetched);
        }
        return 1;
    }        
    else
    {
        if(!isPrefetched) 
        {
            miss++;
            stat_heart_beat_miss++;
        }
        unsigned int victimWayIndex = victimize(setIndex);
        if(cache[setIndex][victimWayIndex].valid)
            eviction(setIndex,victimWayIndex);
        insertion(pc,setIndex,victimWayIndex,tag,isPrefetched);
        return -1;
    }
    
}



void LRUCache::link_wssc(wssc *w, bool w_on)
{
    wssc_map = w;
    wssc_on = w_on;
    wssc_interval = w->get_interval();
}

void LRUCache::insert_early_prefetch_map(unsigned int setIndex, unsigned int wayIndex)
{
    unsigned int lineAddr = ((cache[setIndex][wayIndex].tag << (setsInPowerOfTwo + offset)) + (setIndex << offset));
    std::unordered_map<unsigned int, ep_t*>::iterator it = early_prefetch.find(lineAddr);
    if(it != early_prefetch.end())
    {
        if(it->second->pc == cache[setIndex][wayIndex].pc)
        {
            it->second->count++;
            return;
        }
    }
    ep_t *temp = (ep_t*)malloc(sizeof(ep_t));
    ASSERT(temp!=NULL, "Early prefetch entry creation failed!\n");
    temp->pc = cache[setIndex][wayIndex].pc;
    temp->count = 1;
    early_prefetch.insert(std::pair<unsigned int, ep_t*>(lineAddr, temp));
}

void LRUCache::update_early_prefetch_map(unsigned int pc, unsigned int lineAddr)
{
    std::unordered_map<unsigned int,stat_t*>::iterator mi = demand_miss_map.find(pc);
    if(mi == demand_miss_map.end())
    {
        stat_t *temp = (stat_t*)malloc(sizeof(stat_t));
        temp->counter1 = 1;
        temp->counter2 = 0;
        temp->counter3 = 0;
        demand_miss_map.insert(std::pair<unsigned int,stat_t*>(pc,temp));
    }
    else
        mi->second->counter1++;

    
    std::unordered_map<unsigned int, ep_t*>::iterator it = early_prefetch.find(lineAddr);
    if(it != early_prefetch.end())
    {
        stat_total_early_prefetch += it->second->count;
        mi = demand_miss_map.find(pc);
        mi->second->counter2++;
        unsigned int  generating_pc = it->second->pc;
        std::unordered_map<unsigned int, stat_t*>::iterator it2 = pc_prefetch_map.find(generating_pc);
        if(it2 != pc_prefetch_map.end())
            it2->second->counter2 += it->second->count;
        early_prefetch.erase(it);
    }
}

void LRUCache::insert_pc_prefetch_map(unsigned int pc)
{
    std::unordered_map<unsigned int, stat_t*>::iterator it = pc_prefetch_map.find(pc);
    if(it != pc_prefetch_map.end())
        it->second->counter1++;
    else
    {
        stat_t *temp = (stat_t*)malloc(sizeof(stat_t));
        temp->counter1 = 1;
        temp->counter2 = 0;
        temp->counter3 = 0;
        pc_prefetch_map.insert(std::pair<unsigned int, stat_t*>(pc,temp));
    }
}

void LRUCache::update_pc_prefetch_map(unsigned int pc)
{
    std::unordered_map<unsigned int, stat_t*>::iterator it = pc_prefetch_map.find(pc);
    if(it != pc_prefetch_map.end())
        it->second->counter3++;
}

void LRUCache::wrap_up()
{
    for(int i=0;i<noOfSets;++i)
    {
        for(int j=0;j<associativity;++j)
        {
            if(cache[i][j].pref && !cache[i][j].pref_hit)
                totalPrefetchedUnusedLines++;
        }
    }
}

void LRUCache::heart_beat_stats(int verbose)
{
    BaseCache::heart_beat_stats(verbose);
}

void LRUCache::final_stats(int verbose)
{
    BaseCache::final_stats(verbose);
    fprintf(stdout, "Prefetched = %d\n", totalPrefetchedLines);
    fprintf(stdout, "PrefetchedUnused = %d [%0.2f]\n", totalPrefetchedUnusedLines, (float)totalPrefetchedUnusedLines/totalPrefetchedLines*100);
    fprintf(stdout, "PerfectPrefetch = %d [%0.2f]\n", perfect_prefetch, (float)perfect_prefetch/totalPrefetchedLines*100);
    if(!lite)fprintf(stdout, "EarlyPrefetch = %d [%0.2f]\n", stat_total_early_prefetch, (float)stat_total_early_prefetch/totalPrefetchedUnusedLines*100);
    if(verbose > 0)
    {
        fprintf(stdout, "[ Cache.%s.DEMAND_MISS_MAP]\n", name);
        std::unordered_map<unsigned int,stat_t*>::iterator it;
        for(it=demand_miss_map.begin();it!=demand_miss_map.end();++it)
            fprintf(stdout, "%*x %*d %*d\n", 7, it->first, 7, it->second->counter1, 7, it->second->counter2);
    }
    if(verbose > 1)
    {
        fprintf(stdout, "[ Cache.%s.PC_PREFETCH_MAP]\n", name);
        std::unordered_map<unsigned int, stat_t*>::iterator it;
        for(it=pc_prefetch_map.begin();it!=pc_prefetch_map.end();++it)
            fprintf(stdout, "%*x %*d %*d %*d\n", 7, it->first, 7, it->second->counter1, 7, it->second->counter3, 7, it->second->counter2);
    }
}




