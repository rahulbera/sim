#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SMSPrefetcher.h"

#define LINE_SIZE_LOG 6
#define REGION_SIZE_LOG 12
#define OFFSET_LOG (REGION_SIZE_LOG - LINE_SIZE_LOG)
#define MAX_OFFSET (1<<(OFFSET_LOG))

unsigned int SMSPrefetcher::log2(unsigned int n)
{
	if(n==1) return 0;
    else return (1+log2(n/2));
}

void SMSPrefetcher::prefetcher_init(char *name, int n)
{

}

void SMSPrefetcher::prefetcher_init(char *s, unsigned int acc_size, unsigned int fil_size, unsigned int pht_size, unsigned int pht_assoc)
{
	strcpy(name,s);
	acc_table_size = acc_size;
	filter_table_size = fil_size;
	pht_table_size = pht_size;
	pht_table_assoc = pht_assoc;
	pht_table_sets = pht_size/pht_assoc;
	pht_table_sets_log = log2(pht_table_sets);
	
	#ifdef DEBUG
		fprintf(stderr, "S:%d A:%d S:%d SL:%d\n", pht_table_size, pht_table_assoc, pht_table_sets, pht_table_sets_log);
		fprintf(stderr, "RL:%d LL:%d OF:%d MO:%d\n", REGION_SIZE_LOG, LINE_SIZE_LOG, OFFSET_LOG, MAX_OFFSET);
	#endif

	filter_table = (entry_t*)malloc(filter_table_size*sizeof(entry_t));
	ASSERT(filter_table!=NULL, "Filter table allocation failed!\n");
	for(int i=0;i<filter_table_size;++i)
	{
		filter_table[i].pattern = (bool*)malloc(MAX_OFFSET*sizeof(bool));
		ASSERT(filter_table[i].pattern!=NULL, "Filter table pattern memory alloc failed\n");
		filter_table[i].age = 0;
		filter_table[i].valid = false;
	}

	acc_table = (entry_t*)malloc(acc_table_size*sizeof(entry_t));
	ASSERT(acc_table!=NULL, "Accumulation table allocation failed!\n");
	for(int i=0;i<acc_table_size;++i)
	{
		acc_table[i].pattern = (bool*)malloc(MAX_OFFSET*sizeof(bool));
		ASSERT(acc_table[i].pattern!=NULL, "Accumulation table pattern memory alloc failed\n");	
		acc_table[i].age = 0;
		acc_table[i].valid = false;
	}

	
	pht_table = (pht_t**)malloc(pht_table_sets * sizeof(pht_t*));
	ASSERT(pht_table!=NULL, "PHT table allocation failed\n");
	for(int i=0;i<pht_table_sets;++i)
	{
		pht_table[i] = (pht_t*)malloc(pht_table_assoc * sizeof(pht_t));
		ASSERT(pht_table[i]!=NULL, "PHT table allocation failed 2.\n");
	}
	for(int i=0;i<pht_table_sets;++i)
	{
		for(int j=0;j<pht_table_assoc;++j)
		{
			pht_table[i][j].pattern = (bool*)malloc(MAX_OFFSET*sizeof(bool));
			ASSERT(pht_table[i][j].pattern!=NULL, "PHT table pattern allocation falied\n");
			pht_table[i][j].age = 0;
			pht_table[i][j].valid = false;
		}
	}
	

	stat_total_prefetch = 0;
	stat_total_acc_to_pht = 0;
	stat_total_pht_table_rep = 0;
}

int SMSPrefetcher::prefetcher_operate(unsigned int pc, unsigned int addr, unsigned int *prefAddr)
{

}

int SMSPrefetcher::prefetcher_operate(unsigned int pc, unsigned int addr, unsigned int **prefAddr, int *size)
{
	unsigned int region = addr >> REGION_SIZE_LOG;
	unsigned int offset = ((addr>>LINE_SIZE_LOG) & (MAX_OFFSET-1));
	int index;

	#ifdef DEBUG 
		fprintf(stderr, "R:%x O:%d\n", region, offset);
	#endif
	/* Search for the entry in accumulation table */
	index = -1;
	for(int i=0;i<acc_table_size;++i)
	{
		if(acc_table[i].valid && (acc_table[i].tag == region))
		{
			index = i;
			break;
		}
	}
	if(index != -1) /* Hit in accumulation table */
	{
		#ifdef DEBUG
			fprintf(stderr, "Hit acc table:%d\n", index);
			debug_acc_entry(index);
		#endif
		acc_table[index].pattern[offset] = true;
		for(int i=0;i<acc_table_size;++i) acc_table[i].age++;
		acc_table[index].age = 0;
		#ifdef DEBUG 
			debug_acc_entry(index);
		#endif
		return -1;
	}

	/* If it is not found in accumulation table, search in filter table */
	#ifdef DEBUG 
		fprintf(stderr, "Miss acc table\n");
	#endif
	index = -1;
	for(int i=0;i<filter_table_size;++i)
	{
		if(filter_table[i].valid && (filter_table[i].tag == region))
		{
			index = i;
			break;
		}
	}
	if(index != -1) /* Hit in filter table */
	{
		#ifdef DEBUG
			fprintf(stderr, "Hit fiter table:%d\n", index);
			debug_fil_entry(index);
		#endif
		/* Check if it is the same offset access */
		if(filter_table[index].offset == offset)
		{
			#ifdef DEBUG 
				fprintf(stderr, "Same offset acc, exiting!\n");
			#endif
			return -1;
		}

		/* Else find a replacement candidate in accmulation table */
		int repIndex = -1;
		int maxAge = -1;
		bool foundInvalidEntry = false;
		for(int i=0;i<acc_table_size;++i)
		{
			if(!acc_table[i].valid)
			{
				repIndex = i;
				foundInvalidEntry = true;
				break;
			}
			else if(acc_table[i].age > maxAge)
			{
				maxAge = acc_table[i].age;
				repIndex = i;
			}
		}
		ASSERT(repIndex!=-1, "Invalid victim selection in Accumulation table\n");
		#ifdef DEBUG 
			fprintf(stderr, "Rep index in acc:%d\n", repIndex);
			debug_acc_entry(repIndex);
		#endif
		/* Found an invalid entry in acc table,
		 * no need to copy back the pattern to PHT
		 */
		if(foundInvalidEntry)
		{
			#ifdef DEBUG 
				fprintf(stderr, "%d is invalid\n", repIndex);
			#endif
			acc_table[repIndex].tag = filter_table[index].tag;
			acc_table[repIndex].pc = filter_table[index].pc;
			acc_table[repIndex].offset = filter_table[index].offset;
			for(int i=0;i<MAX_OFFSET;++i) acc_table[repIndex].pattern[i] = filter_table[index].pattern[i];
			acc_table[repIndex].pattern[offset] = true;
			acc_table[repIndex].valid = true;
			for(int i=0;i<acc_table_size;++i) acc_table[i].age++;
			acc_table[repIndex].age = 0;
			filter_table[index].valid = false;
			#ifdef DEBUG 
				debug_acc_entry(repIndex);
			#endif
			return -1;
		}

		/* We got a VALID victim from accumulation table
		 * now store it in PHT. For that, search in PHT
		 * If an entry is already present in PHT for
		 * same PC/Offset combo, update the learned pattern.
		 * Else find a victim, and store the new entry.
		 */
		stat_total_acc_to_pht++;
		int repIndex2 = -1;
		int maxAge2 = -1;
		unsigned long int temp = (acc_table[repIndex].pc << OFFSET_LOG) + acc_table[repIndex].offset;
		unsigned int setIndex = temp & (pht_table_sets - 1);
		unsigned int tag = temp >> pht_table_sets_log;

		for(int i=0;i<pht_table_assoc;++i)
		{
			if(pht_table[setIndex][i].valid && (pht_table[setIndex][i].tag == tag))
			{
				repIndex2 = i;
				break;
			}
		}
		if(repIndex2 != -1) /* Hit in PHT */
		{
			#ifdef DEBUG 
				fprintf(stderr, "Hit in PHT:%d\n", repIndex2); 
				debug_pht_entry(setIndex,repIndex2);
			#endif
			for(int i=0;i<MAX_OFFSET;++i) pht_table[setIndex][repIndex2].pattern[i] = acc_table[repIndex].pattern[i];
			for(int i=0;i<pht_table_assoc;++i) pht_table[setIndex][i].age++;
			pht_table[setIndex][repIndex2].age = 0;
			acc_table[repIndex].valid = false;
			#ifdef DEBUG 
				debug_pht_entry(setIndex,repIndex2);
			#endif
		}
		else /* Miss in PHT */
		{
			#ifdef DEBUG 
				fprintf(stderr, "Miss in PHT\n");
			#endif
			repIndex2 = -1;
			for(int i=0;i<pht_table_assoc;++i)
			{
				if(!pht_table[setIndex][i].valid)
				{
					repIndex2 = i;
					break;
				}
				else if(pht_table[setIndex][i].age > maxAge2)
				{
					maxAge2 = pht_table[setIndex][i].age;
					repIndex2 = i;
				}
			}
			ASSERT(repIndex2!=-1, "Invalid victim selection in PHT table\n");
			#ifdef DEBUG 
				fprintf(stderr, "Rep index in PHT:%d\n", repIndex2); 
				debug_pht_entry(setIndex,repIndex2);
			#endif
			if(pht_table[setIndex][repIndex2].valid) stat_total_pht_table_rep++;
			pht_table[setIndex][repIndex2].tag = tag;
			for(int i=0;i<MAX_OFFSET;++i) pht_table[setIndex][repIndex2].pattern[i] = acc_table[repIndex].pattern[i];
			pht_table[setIndex][repIndex2].valid = true;
			for(int i=0;i<pht_table_assoc;++i) pht_table[setIndex][i].age++;
			pht_table[setIndex][repIndex2].age = 0;
			acc_table[repIndex].valid = false;
			#ifdef DEBUG 
				debug_pht_entry(setIndex,repIndex2);
			#endif
		}
		acc_table[repIndex].tag = filter_table[index].tag;
		acc_table[repIndex].pc = filter_table[index].pc;
		acc_table[repIndex].offset = filter_table[index].offset;
		for(int i=0;i<MAX_OFFSET;++i) acc_table[repIndex].pattern[i] = filter_table[index].pattern[i];
		acc_table[repIndex].pattern[offset] = true;
		for(int i=0;i<acc_table_size;++i) acc_table[i].age++;
		acc_table[repIndex].age = 0;
		acc_table[repIndex].valid = true;
		filter_table[index].valid = false;
		#ifdef DEBUG 
			debug_acc_entry(repIndex);
		#endif
		return -1;
		
	}

	/* If we are still here, that means the entry isn't found 
	 * inside accumulation table, or filter table.
	 * which says, it's a start of a new generation.
	 */
	#ifdef DEBUG 
	 	fprintf(stderr, "Misses both table. Trigger access, start of new gen.\n");
	#endif
	
	/* First allocate in filter table */
	int repIndex = -1;
	int maxAge = -1;
	for(int i=0;i<filter_table_size;++i)
	{
		if(!filter_table[i].valid)
		{
			repIndex = i;
			break;
		}
		else if(filter_table[i].age > maxAge)
		{
			maxAge = filter_table[i].age;
			repIndex = i;
		}
	}
	ASSERT(repIndex!=-1, "Invalid victim selection in filter table");
	#ifdef DEBUG 
		fprintf(stderr, "Rep index in fil table:%d\n", repIndex);
		debug_fil_entry(repIndex);
	#endif
	filter_table[repIndex].tag = region;
	filter_table[repIndex].pc = pc;
	filter_table[repIndex].offset = offset;
	for(int i=0;i<MAX_OFFSET;++i) filter_table[repIndex].pattern[i] = false;
	filter_table[repIndex].pattern[offset] = true;
	filter_table[repIndex].valid = true;
	for(int i=0;i<filter_table_size;++i) filter_table[i].age++;
	filter_table[repIndex].age = 0;
	#ifdef DEBUG 
		debug_fil_entry(repIndex);
	#endif

	/* Now check in PHT if there is any 
	 * learnt pattern for this trigger access
	 */
	#ifdef DEBUG 
		fprintf(stderr, "Trigger access, checking PHT\n");
	#endif
	unsigned long int temp = (pc << OFFSET_LOG) + offset;
	unsigned int setIndex = temp & (pht_table_sets - 1);
	unsigned int tag = temp >> pht_table_sets_log;
	index = -1;
	for(int i=0;i<pht_table_assoc;++i)
	{
		if(pht_table[setIndex][i].valid && (pht_table[setIndex][i].tag == tag))
		{
			index = i;
			break;
		}
	}
	if(index == -1)
	{
		#ifdef DEBUG 
			fprintf(stderr, "No learnt pattern found. No prefetch.\n");
		#endif
		return -1;
	}
	#ifdef DEBUG 
		fprintf(stderr, "Access found in PHT:%d\n", index); 
		debug_pht_entry(setIndex,index);
	#endif
	int n = 0;
	for(int i=0;i<MAX_OFFSET;++i) {if(pht_table[setIndex][index].pattern[i]) n++;}
	#ifdef DEBUG 
		fprintf(stderr, "Prefetch %d\n", n);
	#endif
	unsigned int *prefList = (unsigned int*)malloc(n*sizeof(unsigned int));
	ASSERT(prefList!=NULL, "Pref list allocation failed!\n");
	int k = 0;
	for(int i=0;i<MAX_OFFSET;++i)
	{
		if(pht_table[setIndex][index].pattern[i])
		{
			prefList[k] = (region << REGION_SIZE_LOG) + (i << LINE_SIZE_LOG);
			#ifdef DEBUG 
				fprintf(stderr, "%x ", prefList[k]);
			#endif
			k++;
		}
	}
	(*prefAddr) = prefList;
	(*size) = n;
	stat_total_prefetch += n;
	#ifdef DEBUG 
		fprintf(stderr, "\n");
	#endif

	return 1;
}

void SMSPrefetcher::prefetcher_heartbeat_stats()
{

}

void SMSPrefetcher::prefetcher_final_stats()
{
	fprintf(stdout, "[ Prefetcher.%s ]\n", name);
	fprintf(stdout, "Type = SMS\n");
	fprintf(stdout, "Region = %dB\n", 1 << REGION_SIZE_LOG);
	fprintf(stdout, "ACTSize = %d\n", acc_table_size);
	fprintf(stdout, "FTSize = %d\n", filter_table_size);
	fprintf(stdout, "PHTSize = %d\n", pht_table_size);
	fprintf(stdout, "PHTAssoc = %d\n", pht_table_assoc);
	fprintf(stdout, "TotalPrefetch = %d\n", stat_total_prefetch);
	fprintf(stdout, "TotalAccToPHT = %d\n", stat_total_acc_to_pht);
	fprintf(stdout, "TotalPHTRepl = %d\n", stat_total_pht_table_rep);
}

void SMSPrefetcher::prefetcher_destroy()
{
	#ifdef DEBUG 
		fprintf(stderr, "Deallocating prefetcher.%s\n", name);
	#endif
}

void SMSPrefetcher::debug_acc_entry(int index)
{
	fprintf(stderr, "ACC:<");
	fprintf(stderr, "%*x|%*x|%*d|", 5, acc_table[index].tag, 8, acc_table[index].pc, 2, acc_table[index].offset);
	for(int i=0;i<MAX_OFFSET;++i) fprintf(stderr, "%d", acc_table[index].pattern[i]);
	fprintf(stderr, "|%*d|%d>\n", 2, acc_table[index].age, acc_table[index].valid);
}

void SMSPrefetcher::debug_fil_entry(int index)
{
	fprintf(stderr, "FIL:<");
	fprintf(stderr, "%*x|%*x|%*d|", 5, filter_table[index].tag, 8, filter_table[index].pc, 2, filter_table[index].offset);
	for(int i=0;i<MAX_OFFSET;++i) fprintf(stderr, "%d", filter_table[index].pattern[i]);
	fprintf(stderr, "|%*d|%d>\n", 2, filter_table[index].age, filter_table[index].valid);
}

void SMSPrefetcher::debug_pht_entry(int setIndex, int wayIndex)
{
	fprintf(stderr, "PHT:<");
	fprintf(stderr, "%*lx|%*x|%*d|", 	10, pht_table[setIndex][wayIndex].tag, 
										8, (uint)(pht_table[setIndex][wayIndex].tag >> OFFSET_LOG), 
										2, (uint)(pht_table[setIndex][wayIndex].tag & (MAX_OFFSET-1)));
	for(int i=0;i<MAX_OFFSET;++i) fprintf(stderr, "%d", pht_table[setIndex][wayIndex].pattern[i]);
	fprintf(stderr, "|>\n");
}
