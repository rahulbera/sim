#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SMSPrefetcher.h"

#define LINE_SIZE_LOG 6
#define PAGE_SIZE_LOG 12



void SMSPrefetcher::prefetcher_init(char *name, bool d, int n)
{

}

void SMSPrefetcher::prefetcher_init(char *s, bool d, unsigned int acc_size, unsigned int fil_size, unsigned int pht_size)
{
	strcpy(name,s);
	debug = d;
	acc_table_size = acc_size;
	filter_table_size = fil_size;
	pht_table_size = pht_size;

	filter_table = (entry_t*)malloc(filter_table_size*sizeof(entry_t));
	ASSERT(filter_table!=NULL, "Filter table allocation failed!\n");
	for(int i=0;i<filter_table_size;++i)
		filter_table[i].valid = false;

	acc_table = (entry_t*)malloc(acc_table_size*sizeof(entry_t));
	ASSERT(acc_table!=NULL, "Accumulation table allocation failed!\n");
	for(int i=0;i<acc_table_size;++i)
		acc_table[i].valid = false;

	pht_table = (pht_t*)malloc(pht_table_size*sizeof(pht_t));
	ASSERT(pht_table!=NULL, "PHT allocation failed\n");
	for(int i=0;i<pht_table_size;++i)
		pht_table[i].valid = false;

	stat_total_prefetch = 0;
	stat_total_acc_to_pht = 0;
	stat_total_pht_table_rep = 0;
}

int SMSPrefetcher::prefetcher_operate(unsigned int pc, unsigned int addr, unsigned int *prefAddr)
{

}

int SMSPrefetcher::prefetcher_operate(unsigned int pc, unsigned int addr, unsigned int **prefAddr, int *size)
{
	unsigned int page = addr >> PAGE_SIZE_LOG;
	unsigned int line = addr >> LINE_SIZE_LOG;
	unsigned int offset = (line & 63);
	int index;

	if(debug) fprintf(stderr, "P:%x L:%x O:%d\n", page, line, offset);
	/* Search for the entry in accumulation table */
	index = -1;
	for(int i=0;i<acc_table_size;++i)
	{
		if(acc_table[i].valid && (acc_table[i].page == page))
		{
			index = i;
			break;
		}
	}
	if(index != -1) /* Hit in accumulation table */
	{
		if(debug)
		{
			fprintf(stderr, "Hit acc table:%d\n", index);
			debug_acc_entry(index);
		}
		acc_table[index].pattern[offset] = true;
		for(int i=0;i<acc_table_size;++i) acc_table[i].age++;
		acc_table[index].age = 0;
		if(debug) debug_acc_entry(index);
		return -1;
	}

	/* If it is not found in accumulation table, search in filter table */
	if(debug) fprintf(stderr, "Miss acc table\n");
	index = -1;
	for(int i=0;i<filter_table_size;++i)
	{
		if(filter_table[i].valid && (filter_table[i].page == page))
		{
			index = i;
			break;
		}
	}
	if(index != -1) /* Hit in filter table */
	{
		if(debug) 
		{
			fprintf(stderr, "Hit fiter table:%d\n", index);
			if(debug) debug_fil_entry(index);
		}
		/* Check if it is the same offset access */
		if(filter_table[index].offset == offset)
		{
			if(debug) fprintf(stderr, "Same offset acc, exiting!\n");
			return -1;
		}

		/* Else find a replacement candidate in accmulation table */
		int repIndex = -1;
		int maxAge = 0;
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
		if(debug) fprintf(stderr, "Rep index in acc:%d\n", repIndex);
		if(debug) debug_acc_entry(repIndex);
		/* Found an invalid entry in acc table,
		 * no need to copy back the pattern to PHT
		 */
		if(foundInvalidEntry)
		{
			if(debug) fprintf(stderr, "%d is invalid\n", repIndex);
			acc_table[repIndex].page = filter_table[index].page;
			acc_table[repIndex].pc = filter_table[index].pc;
			acc_table[repIndex].offset = filter_table[index].offset;
			for(int i=0;i<64;++i) acc_table[repIndex].pattern[i] = filter_table[index].pattern[i];
			acc_table[repIndex].pattern[offset] = true;
			acc_table[repIndex].valid = true;
			for(int i=0;i<acc_table_size;++i) acc_table[i].age++;
			acc_table[repIndex].age = 0;
			filter_table[index].valid = false;
			if(debug) debug_acc_entry(repIndex);
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
		int maxAge2 = 0;
		unsigned long int tag = (acc_table[repIndex].pc << 6) + acc_table[repIndex].offset;
		for(int i=0;i<pht_table_size;++i)
		{
			if(pht_table[i].valid && (pht_table[i].tag == tag))
			{
				repIndex2 = i;
				break;
			}
		}
		if(repIndex2 != -1) /* Hit in PHT */
		{
			if(debug) {fprintf(stderr, "Hit in PHT:%d\n", repIndex2); debug_pht_entry(repIndex2);}
			for(int i=0;i<64;++i) pht_table[repIndex2].pattern[i] = acc_table[repIndex].pattern[i];
			for(int i=0;i<pht_table_size;++i) pht_table[i].age++;
			pht_table[repIndex2].age = 0;
			acc_table[repIndex].valid = false;
			if(debug) debug_pht_entry(repIndex2);
		}
		else /* Miss in PHT */
		{
			if(debug) fprintf(stderr, "Miss in PHT\n");
			repIndex2 = -1;
			for(int i=0;i<pht_table_size;++i)
			{
				if(!pht_table[i].valid)
				{
					repIndex2 = i;
					break;
				}
				else if(pht_table[i].age > maxAge2)
				{
					maxAge2 = pht_table[i].age;
					repIndex2 = i;
				}
			}
			ASSERT(repIndex2!=-1, "Invalid victim selection in PHT table\n");
			if(debug) {fprintf(stderr, "Rep index in PHT:%d\n", repIndex2); debug_pht_entry(repIndex2);}
			if(pht_table[repIndex2].valid) stat_total_pht_table_rep++;
			pht_table[repIndex2].tag = tag;
			for(int i=0;i<64;++i) pht_table[repIndex2].pattern[i] = acc_table[repIndex].pattern[i];
			pht_table[repIndex2].valid = true;
			for(int i=0;i<pht_table_size;++i) pht_table[i].age++;
			pht_table[repIndex2].age = 0;
			acc_table[repIndex].valid = false;
			if(debug) debug_pht_entry(repIndex2);
		}
		acc_table[repIndex].page = filter_table[index].page;
		acc_table[repIndex].pc = filter_table[index].pc;
		acc_table[repIndex].offset = filter_table[index].offset;
		for(int i=0;i<64;++i) acc_table[repIndex].pattern[i] = filter_table[index].pattern[i];
		acc_table[repIndex].pattern[offset] = true;
		acc_table[repIndex].age = 0;
		acc_table[repIndex].valid = true;
		for(int i=0;i<acc_table_size;++i) {if(i!=repIndex) acc_table[i].age++;}
		filter_table[index].valid = false;
		if(debug) debug_acc_entry(repIndex);
		return -1;
		
	}

	/* If we are still here, that means the entry isn't found 
	 * inside accumulation table, or filter table.
	 * which says, it's a start of a new generation.
	 */
	if(debug) fprintf(stderr, "Misses both table. Trigger access, start of new gen.\n");
	/* First allocate in filter table */
	int repIndex = -1;
	int maxAge = 0;
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
	if(debug) fprintf(stderr, "Rep index in fil table:%d\n", repIndex);
	if(debug) debug_fil_entry(repIndex);
	filter_table[repIndex].page = page;
	filter_table[repIndex].pc = pc;
	filter_table[repIndex].offset = offset;
	for(int i=0;i<64;++i) filter_table[repIndex].pattern[i] = false;
	filter_table[repIndex].pattern[offset] = true;
	filter_table[repIndex].valid = true;
	for(int i=0;i<filter_table_size;++i) filter_table[i].age++;
	filter_table[repIndex].age = 0;
	if(debug) debug_fil_entry(repIndex);

	/* Now check in PHT if there is any 
	 * learnt pattern for this trigger access
	 */
	if(debug) fprintf(stderr, "Trigger access, checking PHT\n");
	unsigned long int tag = (pc << 6) + offset;
	index = -1;
	for(int i=0;i<pht_table_size;++i)
	{
		if(pht_table[i].valid && (pht_table[i].tag == tag))
		{
			index = i;
			break;
		}
	}
	if(index == -1)
	{
		if(debug) fprintf(stderr, "No learnt pattern found. No prefetch.\n");
		return -1;
	}
	if(debug) {fprintf(stderr, "Access found in PHT:%d\n", index); debug_pht_entry(index);}
	int n = 0;
	for(int i=0;i<64;++i) {if(pht_table[index].pattern[i]) n++;}
	if(debug) fprintf(stderr, "Prefetch %d\n", n);
	unsigned int *prefList = (unsigned int*)malloc(n*sizeof(unsigned int));
	ASSERT(prefList!=NULL, "Pref list allocation failed!\n");
	int k = 0;
	for(int i=0;i<64;++i)
	{
		if(pht_table[index].pattern[i])
		{
			prefList[k] = (page << 12) + (i << 6);
			if(debug) fprintf(stderr, "%x ", prefList[k]);
			k++;
		}
	}
	(*prefAddr) = prefList;
	(*size) = n;
	stat_total_prefetch += n;
	if(debug) fprintf(stderr, "\n");

	return 1;
}

void SMSPrefetcher::prefetcher_heartbeat_stats()
{

}

void SMSPrefetcher::prefetcher_final_stats()
{
	fprintf(stdout, "[ Prefetcher.%s ]\n", name);
	fprintf(stdout, "Type = SMS\n");
	fprintf(stdout, "ACTSize = %d\n", acc_table_size);
	fprintf(stdout, "FTSize = %d\n", filter_table_size);
	fprintf(stdout, "PHTSize = %d\n", pht_table_size);
	fprintf(stdout, "TotalPrefetch = %d\n", stat_total_prefetch);
	fprintf(stdout, "TotalAccToPHT = %d\n", stat_total_acc_to_pht);
	fprintf(stdout, "TotalPHTRepl = %d\n", stat_total_pht_table_rep);
}

void SMSPrefetcher::prefetcher_destroy()
{
	if(debug) fprintf(stderr, "Deallocating prefetcher.%s\n", name);
}

void SMSPrefetcher::debug_acc_entry(int index)
{
	fprintf(stderr, "ACC:<");
	fprintf(stderr, "%*x|%*x|%*d|", 5, acc_table[index].page, 8, acc_table[index].pc, 2, acc_table[index].offset);
	for(int i=0;i<64;++i) fprintf(stderr, "%d", acc_table[index].pattern[i]);
	fprintf(stderr, "|%*d|%d>\n", 2, acc_table[index].age, acc_table[index].valid);
}

void SMSPrefetcher::debug_fil_entry(int index)
{
	fprintf(stderr, "FIL:<");
	fprintf(stderr, "%*x|%*x|%*d|", 5, filter_table[index].page, 8, filter_table[index].pc, 2, filter_table[index].offset);
	for(int i=0;i<64;++i) fprintf(stderr, "%d", filter_table[index].pattern[i]);
	fprintf(stderr, "|%*d|%d>\n", 2, filter_table[index].age, filter_table[index].valid);
}

void SMSPrefetcher::debug_pht_entry(int index)
{
	fprintf(stderr, "PHT:<");
	fprintf(stderr, "%*lx|%*x|%*d|", 10, pht_table[index].tag, 8, (uint)(pht_table[index].tag >> 6), 2, (uint)(pht_table[index].tag & 63));
	for(int i=0;i<64;++i) fprintf(stderr, "%d", acc_table[index].pattern[i]);
	fprintf(stderr, "|>\n");
}