#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include "prefetcher/SMSPrefetcher.h"
#include "prefetcher/wssc.h"
#include "cache/LRUCache.h"
#include "cache/DRRIPCache.h"
#include "common/vars.h"

static char trace_file_name[1000];
static gzFile gfp;
static bool trace_dump;
static bool l1trace_dump;
static bool l2trace_dump;
static int  heart_beat;
static bool hide_heart_beat;
static bool hasBegin;
static bool hasEnd;
static bool prefetcher_on;
static bool cache_on;
static int  l1_verbose;
static int  l2_verbose;
static bool cache_lite;
static unsigned int begin, end;
static unsigned int count; 
static char help[1000] = 
				"Supported knobs:\n"
				"--trace <file>\n"
				"	It's a mandatory knob. Sim will not run without a trace file.\n"
				"--hearbeat <int>\n"
				"	Sets heartbeat interval to supplied value. Default 1,000,000\n"
				"--begin <n>\n"
				"	Begins trace processing from here. Default 1\n"
				"--end <n>\n"
				"	Ends trace processing here. Default till end.\n"
				"--trace-dump\n"
				"	Dumps memtrace in stderr. Not compatible with other knobs except trace.\n"
				"	Disables cache and prefetcher by default. Hides heartbeat.\n"
				"--l1trace-dump\n"
				"	Dumps l1miss memtrace in stderr. Don't use it with other trace dumps.\n"
				"	Enables cache by default. Hides heartbeat.\n"
				"--l2trace-dump\n"
				"	Dumps l2miss memtrace in stderr. Don't use it with other trace dumps.\n"
				"	Enables cache by default. Hides heartbeat.\n"
				"--[l1|l2]verbose <n>\n"
				"	Cache verbose level, from 0-4. Default 0.\n"
				"--expand-cache-stat\n"
				"	Gives a broader idea of cache stats. Default false.\n"
				"--hide-heartbeat\n"
				"	Hides heart beat stats.\n"
				"--no-prefetch\n"
				"	Disables prefetcher.\n"
				"--no-cache\n"
				"	Disables cache hierarchy.\n"
				;

static LRUCache l1;
#if L2_TYPE == 1
	static LRUCache l2;
#else
	static DRRIPCache l2;
#endif
static SMSPrefetcher smsp;
static wssc wssc_map;

void sim_parse_command_line(int argc, char *argv[])
{
	if(argc==1)
	{
		fprintf(stderr, "%s\n", help);
		exit(1);
	}

	int i;
	for(i=1; i<argc; ++i)
	{
		if(!strcmp(argv[i], "--help"))
		{
			fprintf(stderr, "%s\n", help);
			exit(0);
		}
		if(!strcmp(argv[i], "--trace"))
		{
			strcpy(trace_file_name, argv[++i]);
			continue;
		}
		if(!strcmp(argv[i], "--trace-dump"))
		{
			if(l1trace_dump || l2trace_dump)
			{
				fprintf(stderr, "Fatal: Don't use trace-dump with other trace dumps\n");
				exit(1);
			}
			trace_dump = true;
			prefetcher_on = false;
			cache_on = false;
			hide_heart_beat = true;
			continue;
		}
		if(!strcmp(argv[i], "--l1trace-dump"))
		{
			if(trace_dump)
			{
				fprintf(stderr, "Fatal: Don't use l1trace-dump with other trace dumps\n");
				exit(1);
			}
			l1trace_dump = true;
			cache_on = true;
			hide_heart_beat = true;
			continue;
		}
		if(!strcmp(argv[i], "--l2trace-dump"))
		{
			if(trace_dump)
			{
				fprintf(stderr, "Fatal: Don't use l2trace-dump and trace-dump simultaneously\n");
				exit(1);
			}
			l2trace_dump = true;
			cache_on = true;
			hide_heart_beat = true;
			continue;
		}
		if(!strcmp(argv[i], "--l1verbose"))
		{
			l1_verbose = atoi(argv[++i]);
			continue;
		}
		if(!strcmp(argv[i], "--l2verbose"))
		{
			l2_verbose = atoi(argv[++i]);
			continue;
		}
		if(!strcmp(argv[i], "--expand-cache-stat"))
		{
			cache_lite = false;
			continue;
		}
		if(!strcmp(argv[i], "--hide-heartbeat"))
		{
			hide_heart_beat = true;
			continue;
		}
		if(!strcmp(argv[i], "--heartbeat"))
		{
			heart_beat = atoi(argv[++i]);
			continue;
		}
		if(!strcmp(argv[i], "--begin"))
		{
			hasBegin = true;
			begin = atoi(argv[++i]);
			continue;
		}
		if(!strcmp(argv[i], "--end"))
		{
			hasEnd = true;
			end = atoi(argv[++i]);
			continue;
		}
		if(!strcmp(argv[i], "--no-prefetch"))
		{
			prefetcher_on = false;
			continue;
		}
		if(!strcmp(argv[i], "--no-cache"))
		{
			cache_on = false;
			continue;
		}
		
		fprintf(stderr, "Invalid knob %s\n", argv[i]);
		fprintf(stderr, "%s\n", help);
		exit(1);
	}
}

void sim_init()
{
	trace_dump = false;
	l1trace_dump = false;
	l2trace_dump = false;
	hide_heart_beat = false;
	heart_beat = 10000000;
	hasBegin = false;
	hasEnd = false;
	prefetcher_on = true;
	cache_on = true;
	l1_verbose = 0;
	l2_verbose = 0;
	cache_lite = true;
	count = 0;
}

void sim_fini(int signum)
{
	if(cache_on)
	{
		l1.wrap_up();
		l2.wrap_up();
		l1.final_stats(l1_verbose);
		l2.final_stats(l2_verbose);
	}

	if(prefetcher_on)
		smsp.prefetcher_final_stats();

	wssc_map.final_stats();

	if(prefetcher_on)
		smsp.prefetcher_destroy();

	gzclose(gfp);

	exit(signum);
}


int main(int argc, char *argv[])
{
	signal(SIGINT, sim_fini);

	sim_init();
	sim_parse_command_line(argc, argv);

	#ifdef DEBUG
		fprintf(stderr, "Trace file: %s\n", trace_file_name);
	#endif

	gfp = gzopen(trace_file_name,"r");
	ASSERT(gfp!=Z_NULL,"Can't open file\n");

	/* SYSTEM DECLARATION BEGIN */

	l1.init("L1D", "LRU", L1D_A, L1D_B, L1D_C, cache_lite);
	#if L2_TYPE == 1
		l2.init("L2", "LRU", L2_A, L2_B, L2_C, cache_lite);
	#else
		l2.init("L2", "DRRIP", L2_A, L2_B, L2_C, RRPV_SIZE, THROTTLE, PSEL_SIZE, cache_lite);
	#endif
	smsp.prefetcher_init("SMS", SMS_ACC, SMS_FIL, SMS_PHT, SMS_PHT_A);
	wssc_map.init("SMS", PLT_S, PLT_A, SAT_A, WSSC_I, THRESHOLD);
	wssc_map.link_prefetcher(&smsp);
	l2.link_wssc(&wssc_map);
	smsp.link_wssc(&wssc_map);

	/* SYSTEM DECALARATION END */

	unsigned int pc, addr;
	unsigned int *prefList; int size;
	bool isRead;
	int n, res, foundInL1, foundInL2;
	fprintf(stderr, "Press Ctrl+C to stop simulation\n");
	while(gzread(gfp,(void*)(&pc),sizeof(unsigned int)) != 0)
	{
		count++;
		n = gzread(gfp,(void*)(&addr),sizeof(unsigned int));
		ASSERT(n!=0,"Addr record reading failed!\n");
		n = gzread(gfp,(void*)(&isRead),sizeof(bool));
		ASSERT(n!=0,"IsBool record reading failed!\n");
		
		if(hasBegin && count<begin)
			continue;
		if(hasEnd && count>end)
			break;
		
		if (trace_dump) 
		{
			fprintf(stderr, "%*x %*x %*d P:%*x L:%*x LO:%*d\n", 7, pc, 8, addr, 1, isRead, 5, addr>>12, 7, addr>>6, 2, (addr>>6)&63);
			continue;
		}

		#ifdef DEBUG
			fprintf(stderr, "[ No:%d ]\n%x %x %d\n", count, pc, addr, isRead);		
		#endif
		
		if(cache_on)
		{
			foundInL1 = l1.update(pc, addr, false);
			if(foundInL1 == -1 && l1trace_dump)
					fprintf(stderr, "%*x %*x %*d P:%*x L:%*x LO:%*d\n", 7, pc, 8, addr, 1, isRead, 5, addr>>12, 7, addr>>6, 2, (addr>>6)&63);
			if(foundInL1 == -1)
			{
				foundInL2 = l2.update(pc, addr, false);
				if(foundInL2 == -1 && l2trace_dump)
					fprintf(stderr, "%*x %*x %*d P:%*x L:%*x LO:%*d\n", 7, pc, 8, addr, 1, isRead, 5, addr>>12, 7, addr>>6, 2, (addr>>6)&63);
			}
		}

		if(prefetcher_on)
		{
			res = smsp.prefetcher_operate(pc, addr, (&prefList), &size);
			if(res != -1 && cache_on)
			{
				for(int i=0;i<size;++i)
					l2.update(pc, prefList[i], true);
			}
		}
		

		if(!hide_heart_beat && (count%heart_beat)==0)
		{
			fprintf(stderr, "Processed: %*d ", 3, count/heart_beat);
			if(prefetcher_on) 
				smsp.prefetcher_heartbeat_stats();
			if(cache_on)
			{
				l2.heart_beat_stats(0);
			}
			fprintf(stderr,"\n");
		}
		
	}

	sim_fini(0);
}
