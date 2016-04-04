#ifndef SMSPREFETCHER_H
#define SMSPREFETCHER_H

#include <stdbool.h>
#include "Prefetcher.h"
#include "wssc.h"

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
	unsigned int tc, uc;
	int age;
	bool valid;
}pht_t;

class wssc;

class SMSPrefetcher : public Prefetcher
{
	private:
		char name[100];
		wssc *wssc_map;
		bool wssc_on;

		entry_t *acc_table;
		entry_t *filter_table;
		pht_t **pht_table;
		unsigned int acc_table_size;
		unsigned int filter_table_size;
		unsigned int pht_table_size;
		unsigned int pht_table_sets;
		unsigned int pht_table_sets_log;
		unsigned int pht_table_assoc;
		float wssc_threshold;

		/* Stats*/
		unsigned int stat_total_prefetch;
		unsigned int stat_total_pht_hit;
		unsigned int stat_total_threshold_check_failure;
		unsigned int stat_total_acc_to_pht;
		unsigned int stat_total_pht_table_rep;
		unsigned int stat_total_pht_pattern_update;
		unsigned int tc_max, uc_max;

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

		void link_wssc(wssc*, bool);
		void incr_tc(unsigned long int, unsigned int);
		void decr_tc(unsigned long int, unsigned int);
		void incr_uc(unsigned long int);
		void update_tc_uc();

		void debug_acc_entry(int);
		void debug_fil_entry(int);
		void debug_pht_entry(int,int);
		void dump(unsigned int);

		unsigned int log2(unsigned int);
};

#endif /*SMSPREFETCHER_H*/