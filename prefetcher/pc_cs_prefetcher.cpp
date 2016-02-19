#include <stdio.h>
#include <stdlib.h>
#include "prefetcher.h"

/*	This file impletements PC/CS prefetcher
 *	based on HPCA'04 paper by Nesbit & Smith
 */

#define it_size 16
#define ghb_size 256
#define it_size_log 8

enum pointer_type
{
	invalid = 0,
	ptr_ghb,
	ptr_it,
};

typedef struct ghb_entry
{
 	unsigned int addr;
 	int predecessor;
 	int pointed_by;
 	pointer_type pointed_by_type;
}ghb_entry_t;

typedef struct it_entry
{
 	unsigned int tag;
 	int ptr;
 	int age;
}it_entry_t;

it_entry_t index_table[it_size];
ghb_entry_t ghb[ghb_size];

int ghb_head;
unsigned int prevPrefetchAddr;

/* Stats counters */
unsigned int stat_totalMemAccess = 0;
unsigned int stat_totalPrediction = 0;
unsigned int stat_totalHitPrediction = 0;

void get_it_index_tag(unsigned int value, unsigned int *index, unsigned int *tag)
{
	/* This is used for direct mapped */
	//(*index) = value & (it_size - 1);
	//(*tag) = value >> it_size_log;

	/* This is used for fully associative */
	(*tag) = value;
	int repIndex = -1;
	int maxAge = -1, maxAgeIndex = -1;
	for(int i=0;i<it_size;++i)
	{
		if(index_table[i].age == -1)
			repIndex = i;
		else
		{
			if(index_table[i].tag == value)
				repIndex = i;
			if(index_table[i].age > maxAge)
			{
				maxAge = index_table[i].age;
				maxAgeIndex = i;
			}
			if(index_table[i].age != -1)
				index_table[i].age++;
		}
	}
	if(repIndex != -1)
	{
		index_table[repIndex].age = 0;
		(*index) = (unsigned int)repIndex;
		return;
	}
	ASSERT(maxAgeIndex>=0, "Invalid replacement index calculation\n");
	index_table[maxAgeIndex].age = 0;
	(*index) = (unsigned int)maxAgeIndex;
	return;

}

void print_ghb()
{
	fprintf(stdout,"=========== GHB ===========\n");
	for(int i=0;i<ghb_size;++i)
		fprintf(stdout,"%x\t%d\t%d\t%d\n", ghb[i].addr, ghb[i].predecessor, ghb[i].pointed_by, ghb[i].pointed_by_type);
	fprintf(stdout,"===========================\n");
}

void prefetcher_init()
{
 	for(int i=0;i<it_size;++i)
 	{
 		index_table[i].tag = 0;
 		index_table[i].ptr = -1;
 		index_table[i].age = -1;
 	}
 	for(int i=0;i<ghb_size;++i)
 	{
 		ghb[i].addr = 0;
 		ghb[i].predecessor = -1;
 		ghb[i].pointed_by = -1;
 		ghb[i].pointed_by_type = invalid;
 	}
 	ghb_head = 0;
}

int prefetcher_operate(unsigned int pc, unsigned int addr, unsigned int *prefAddr)
{
	stat_totalMemAccess++;
	if(addr == prevPrefetchAddr)
		stat_totalHitPrediction++;

 	unsigned int it_index, it_tag;
 	get_it_index_tag(pc, &it_index, &it_tag);
	//fprintf(stderr,"Index: %d Tag: %x\n", it_index, it_tag);

	//it_entry_t 	index_table[it_index] 	= index_table[it_index];
	//ghb_entry_t ghb[ghb_head] 	= ghb[ghb_head];
 	
 	/* 	Check in the index table first
 	 *	If it's a index table miss, assign new tag
 	 *	But first ensure if it is already
 	 *	pointing to a linked list
 	 */
 	if(it_tag != index_table[it_index].tag)
 	{
 		if(index_table[it_index].ptr != -1)
 		{
 			ASSERT(ghb[index_table[it_index].ptr].pointed_by == (int)it_index, "pointer mismatch: it->ghb->it");
 			ghb[index_table[it_index].ptr].pointed_by = -1;
 			ghb[index_table[it_index].ptr].pointed_by_type = invalid;
 		}
 		index_table[it_index].tag = it_tag;
 		index_table[it_index].ptr = -1;
 	}

 	/*  GHB Updation phase 
 	 * 	Check whether the current ghb entry pointed by ghb_head
	 *	is already pointed by another ghb_entry_t or it_entry_t
	 */
 	if(ghb[ghb_head].pointed_by != -1)
 	{
 		/* Check if this record is pointed by ghb */
 		if(ghb[ghb_head].pointed_by_type == ptr_ghb)
 		{
 			ASSERT(ghb[ghb[ghb_head].pointed_by].predecessor == ghb_head, "pointer mismatch: ghb->ghb->ghb");
 			ghb[ghb[ghb_head].pointed_by].predecessor = -1;
 		}
 		/* Check if this is pointed by index_table entry */
 		else if(ghb[ghb_head].pointed_by_type == ptr_it)
 		{
 			ASSERT(index_table[ghb[ghb_head].pointed_by].ptr == ghb_head, "pointer mismatch: ghb->it->ghb");
 			index_table[ghb[ghb_head].pointed_by].ptr = -1;
 		}
 	}

 	/* 	Insert the new mem access entry into ghb
 	 * 	If a linked list exists for this it_index already,
 	 * 	add the new access to the head of the linked list
 	 */
 	ghb[ghb_head].addr = addr;
 	if(index_table[it_index].ptr != -1)
 	{
 		ghb[ghb_head].predecessor = index_table[it_index].ptr;
 		ghb[ghb[ghb_head].predecessor].pointed_by = ghb_head;
 		ghb[ghb[ghb_head].predecessor].pointed_by_type = ptr_ghb;
 		ghb[ghb_head].pointed_by = it_index;
 		ghb[ghb_head].pointed_by_type = ptr_it;
 		index_table[it_index].ptr = ghb_head;
 	}
 	/* If it is the first entry, create a new linked list */
 	else
 	{
 		ghb[ghb_head].predecessor = -1;
 		ghb[ghb_head].pointed_by = it_index;
 		ghb[ghb_head].pointed_by_type = ptr_it;
 		index_table[it_index].ptr = ghb_head;
 	}

 	/* Update ghb head pointer */
 	ghb_head++;
 	if(ghb_head >= ghb_size)
 		ghb_head = ghb_head % ghb_size;

	//if((stat_totalMemAccess%5) == 0) print_ghb();

 	/* 	Now its time for prefetch address prediction */
 	/* 	Check if its the first or second occurence
 	 * 	of the same PC. If yes, don't prefetch
 	 */
 	if((ghb[index_table[it_index].ptr].predecessor == -1) || (ghb[ghb[index_table[it_index].ptr].predecessor].predecessor == -1))
 		return -1;
 	int stride1 = ghb[ghb[index_table[it_index].ptr].predecessor].addr - ghb[index_table[it_index].ptr].addr;
 	int stride2 = ghb[ghb[ghb[index_table[it_index].ptr].predecessor].predecessor].addr - ghb[ghb[index_table[it_index].ptr].predecessor].addr;
 	if(stride1 != stride2)
 		return -1;

 	//fprintf(stderr,"Code reached here\n");	
 	(*prefAddr) = addr + stride1;
 	prevPrefetchAddr = (*prefAddr);
 	stat_totalPrediction++;
 	return 1;
}

void prefetcher_heartbeat_stats()
{
	
}

void prefetcher_final_stats()
{
	fprintf(stdout, "\nTotal Memory Access: %d\n", stat_totalMemAccess);
	fprintf(stdout, "Total prefetcher prediction: %d [%0.2f]\n", stat_totalPrediction, (float)stat_totalPrediction/stat_totalMemAccess*100);
	fprintf(stdout, "Total prediction hit: %d [%0.2f]\n", stat_totalHitPrediction, (float)stat_totalHitPrediction/stat_totalPrediction*100);
}

void prefetcher_destroy()
{
	fprintf(stderr, "Deallocating PC/CS prefetcher\n");
}
