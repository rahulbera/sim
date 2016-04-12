#include "DRRIPCache.h"



DRRIPCache::DRRIPCache()
{
    
}

DRRIPCache::~DRRIPCache()
{
    
}

void DRRIPCache::release()
{
    BaseCache::release();
    for(uint i=0;i<noOfSets;++i)
        free(rrpv[i]);
    free(rrpv);
}

void DRRIPCache::init(char *s, char *t, uint a, uint b, uint c, uint n, uint p, uint m, bool isLite)
{
    BaseCache::init(s,t,a,b,c);
    lite = isLite;
    maxRRPV = (1<<n)-1;
    rrpv = (uint**)malloc(noOfSets*sizeof(uint*));
    for(uint i=0;i<noOfSets;++i)
        rrpv[i] = (uint*)malloc(associativity*sizeof(uint));
    for(uint i=0;i<noOfSets;++i)
    {
        for(uint j=0;j<associativity;++j)
            rrpv[i][j] = maxRRPV;
    }
    
    throttleCounter = 0;
    throttleParam = p;
    psel = Monitor();
    psel.set(m);    

    wssc_map = NULL;
    wssc_interval = 0;
    stat_total_early_prefetch = 0;
    totalPrefetchedLines = totalPrefetchedUnusedLines = 0;
    perfect_prefetch = 0;
    miss_prediction = 0;
}

int DRRIPCache::find(uint setIndex, uint tag)
{
    for(uint i=0;i<associativity;++i)
    {
        if(cache[setIndex][i].tag==tag && cache[setIndex][i].valid)
            return i;
    }
    return -1;
}

void DRRIPCache::promotion(uint setIndex, uint wayIndex, bool isPrefetched)
{
    rrpv[setIndex][wayIndex] = 0;
    cache[setIndex][wayIndex].reuseCount++;

    if(cache[setIndex][wayIndex].pref && !cache[setIndex][wayIndex].pref_hit)
    {
        perfect_prefetch++;
        cache[setIndex][wayIndex].pref_hit = true;
        //if(!lite) 
        //    update_pc_prefetch_map(cache[setIndex][wayIndex].pc);
    }
}

uint DRRIPCache::victimize(uint setIndex)
{
    uint idx=0;
    bool foundVictim = false;
    while(!foundVictim)
    {
        for(uint i=0;i<associativity;++i)
        {
            if(!cache[setIndex][i].valid || (rrpv[setIndex][i]==maxRRPV && cache[setIndex][i].valid))
            {
                foundVictim = true;
                idx = i;
                break;
            }
        }
        if(!foundVictim)
        {
            for(uint i=0;i<associativity;++i)
                rrpv[setIndex][i]++;
        }
        else
            break;
    }    
    return idx;
}

void DRRIPCache::eviction(uint setIndex, uint wayIndex)
{
    if(cache[setIndex][wayIndex].valid)
    {
        if(cache[setIndex][wayIndex].reuseCount>=15)
            reuseCountBucket[15]++;
        else
            reuseCountBucket[cache[setIndex][wayIndex].reuseCount]++;
    }    
    
    if(cache[setIndex][wayIndex].pref && !cache[setIndex][wayIndex].pref_hit)
    {
        totalPrefetchedUnusedLines++;
        //if(!lite)
        //    insert_early_prefetch_map(setIndex, wayIndex);
    }

    cache[setIndex][wayIndex].valid = false;
}

void DRRIPCache::insertion(uint pc, uint setIndex, uint wayIndex, uint tag, bool isPrefetched)
{

}

void DRRIPCache::insertion(uint pc, uint setIndex, uint wayIndex, uint tag, bool isSRRIP, bool isPrefetched)
{
    unsigned int lineAddr = ((tag << (setsInPowerOfTwo + offset)) + (setIndex << offset));
    
    //if(!isPrefetched && !lite)
    //    update_early_prefetch_map(pc, lineAddr);

    if(isPrefetched)
    {
        totalPrefetchedLines++;
        //if(!lite)
        //    insert_pc_prefetch_map(pc);
    }

    cache[setIndex][wayIndex].tag = tag;
    cache[setIndex][wayIndex].pc = pc;
    cache[setIndex][wayIndex].valid = true;
    cache[setIndex][wayIndex].reuseCount = 0;
    cache[setIndex][wayIndex].pref = isPrefetched;
    cache[setIndex][wayIndex].pref_hit = false;
    if(throttleCounter==throttleParam || isSRRIP)
    {
        rrpv[setIndex][wayIndex] = (maxRRPV - 1);
        if(throttleCounter == throttleParam)
            throttleCounter = 0;
        else
            throttleCounter++;
    }
    else
    {
        rrpv[setIndex][wayIndex] = maxRRPV;
        throttleCounter++;
    }
}

int DRRIPCache::update(uint pc, uint addr, bool isPrefetched)
{
    uint temp = addr >> offset;
    uint setIndex = temp & ((1<<setsInPowerOfTwo)-1);
    uint tag = temp >> setsInPowerOfTwo;
    
    uint first=0,second=0,half=setsInPowerOfTwo/2;
    uint type;
	
    if(setsInPowerOfTwo%2 != 0)
    {
        first = setIndex & ((1<<half)-1);
	   second = setIndex >> (1+half);
    }
    else
    {
    	first = setIndex & ((1<<half)-1);
	   second = setIndex >> half;
    }
    if(first == second)
        type = 1; //A Dedicated SRRIP set
    else if((first+second)==((1<<half)-1))
        type = 2; //A dedicated BRRIP set
    else
        type = 0; //A follower set

    if(!isPrefetched) 
    {
        totalAccess++;
        stat_heart_beat_total_access++;
        if(wssc_map && (totalAccess % wssc_interval) == 0)
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
            return 1;
        }
        
    }        
    else
    {
        if(!isPrefetched) 
        {
            miss++;
            stat_heart_beat_miss++;
        }
        uint victimWayIndex = victimize(setIndex);
        if(cache[setIndex][victimWayIndex].valid)
            eviction(setIndex,victimWayIndex);
        
        if(type == 1)
            psel.inc();
        else if(type == 2)
            psel.dec();
        
        bool isSRRIP;
        if(type == 1)
            isSRRIP = true;
        else if(type == 2)
            isSRRIP = false;
        else
        {
            if (psel.value()>=(psel.max()/2)) //psel value crosses mid val = LRU causes more miss, ie choose BIP
                isSRRIP = false;
            else
                isSRRIP = true;
        }
        insertion(pc, setIndex, victimWayIndex, tag, isSRRIP, isPrefetched);
        
        return -1; 
    }
}

void DRRIPCache::link_wssc(wssc *w)
{
    wssc_map = w;
    wssc_interval = w->get_interval();
}

void DRRIPCache::wrap_up()
{

}

void DRRIPCache::heart_beat_stats(int verbose)
{
    BaseCache::heart_beat_stats(verbose);
}

void DRRIPCache::final_stats(int verbose)
{
    BaseCache::final_stats(verbose);
    fprintf(stdout, "Prefetched = %d\n", totalPrefetchedLines);
    fprintf(stdout, "PrefetchedUnused = %d [%0.2f]\n", totalPrefetchedUnusedLines, (float)totalPrefetchedUnusedLines/totalPrefetchedLines*100);
    fprintf(stdout, "PerfectPrefetch = %d [%0.2f]\n", perfect_prefetch, (float)perfect_prefetch/totalPrefetchedLines*100);
    if(!lite)fprintf(stdout, "EarlyPrefetch = %d [%0.2f]\n", stat_total_early_prefetch, (float)stat_total_early_prefetch/totalPrefetchedUnusedLines*100);
    /*if(verbose > 0)
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
    }*/
}

