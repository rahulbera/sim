#ifndef SMSPREFETCHER_H
#define SMSPREFETCHER_H

#include <stdbool.h>
#include "Prefetcher.h"

typedef struct entry1
{
	unsigned int tag;
	unsigned int pc;
	unsigned int offset;
	bool *pattern;
	int age;
	bool valid;
}entry_t;

typedef struct entry2
{
	unsigned long int tag;
	bool *pattern;
	int age;
	bool valid;
}pht_t;



class SMSPrefetcher : public Prefetcher
{
	private:
		char name[100];
		entry_t *acc_table;
		entry_t *filter_table;
		pht_t **pht_table;
		unsigned int acc_table_size;
		unsigned int filter_table_size;
		unsigned int pht_table_size;
		unsigned int pht_table_sets;
		unsigned int pht_table_sets_log;
		unsigned int pht_table_assoc;

		/* Stats*/
		unsigned int stat_total_prefetch;
		unsigned int stat_total_acc_to_pht;
		unsigned int stat_total_pht_table_rep;

	public:
		SMSPrefetcher(){}
		~SMSPrefetcher(){}
		void prefetcher_init(char *name, int n);
		void prefetcher_init(char*, unsigned int, unsigned int, unsigned int, unsigned int);
		int  prefetcher_operate(unsigned int ip, unsigned int addr, unsigned int *prefAddr);
		int  prefetcher_operate(unsigned int ip, unsigned int addr, unsigned int **prefAddr, int *size);
		void prefetcher_heartbeat_stats();
		void prefetcher_final_stats();
		void prefetcher_destroy();

		void debug_acc_entry(int);
		void debug_fil_entry(int);
		void debug_pht_entry(int,int);

		unsigned int log2(unsigned int);
};

#endif /*SMSPREFETCHER_H*/