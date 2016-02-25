#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include <string.h>
#include <stdbool.h>
#include "prefetcher/StreamPrefetcher.h"
#include "prefetcher/SMSPrefetcher.h"
#include "cache/LRUCache.h"

#define B 1
#define KB (1024*B)
#define MB (1024*KB)

static char trace_file_name[1000];
static bool debug;
static bool trace_dump;
static int heart_beat;
static bool hide_heart_beat;
static bool hasBegin;
static bool hasEnd;
static bool prefetcher_on;
static bool cache_on;
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
				"	Dumps memtrace in stderr.\n"
				"--debug\n"
				"	Enables debugging mode. Dumps debug log in stderr.\n"
				"--hide-heartbeat\n"
				"	Hides heart beat stats.\n"
				"--no-prefetch\n"
				"	Disables prefetcher.\n"
				"--no-cache\n"
				"	Disables cache hierarchy.\n";

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
			trace_dump = true;
			prefetcher_on = false;
			cache_on = false;
			continue;
		}
		if(!strcmp(argv[i], "--debug"))
		{
			debug = true;
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
	debug = false;
	trace_dump = false;
	hide_heart_beat = false;
	heart_beat = 10000000;
	hasBegin = false;
	hasEnd = false;
	prefetcher_on = true;
	cache_on = true;
	count = 0;
}


int main(int argc, char *argv[])
{
	sim_init();
	sim_parse_command_line(argc, argv);

	if(debug)fprintf(stderr, "Trace file: %s\n", trace_file_name);
	gzFile gfp;
	gfp = gzopen(trace_file_name,"r");
	ASSERT(gfp!=Z_NULL,"Can't open file\n");

	/* Cache specification */
	LRUCache l1 = LRUCache("L1D", 4, 64*B, 32*KB);
	LRUCache l2 = LRUCache("L2", 16, 64*B, 2*MB);
	
	/* Prefetcher specification */
	StreamPrefetcher sp;
	sp.prefetcher_init("Uni",debug, 64);
	SMSPrefetcher smsp;
	smsp.prefetcher_init("SMS",debug, 16, 32, 4*1024, 4);

	unsigned int pc, addr, prefAddr;
	unsigned int *prefList; int size;
	bool isRead, foundInL1;
	int n, res;
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

		if (debug) 
			fprintf(stderr, "[ No:%d ]\n%x %x %d\n", count, pc, addr, isRead);		
		
		foundInL1 = true;
		if(cache_on)
		{
			if(l1.find(addr) == -1)
			{
				foundInL1 = false;
				l2.update(addr,false);
			}
			l1.update(addr,false);
		}

		if(prefetcher_on)
		{
			/*res = sp.prefetcher_operate(pc, addr, (&prefAddr));
			if(res != -1 && cache_on) 
				l2.update(prefAddr,true);
			*/
			res = smsp.prefetcher_operate(pc, addr, (&prefList), &size);
			if(res != -1 && cache_on)
			{
				for(int i=0;i<size;++i)
					l2.update(prefList[i],true);
			}
		}
		

		if(!hide_heart_beat && (count%heart_beat)==0)
		{
			fprintf(stderr, "Processed: %*d ", 3, count/heart_beat);
			if(prefetcher_on) 
				//sp.prefetcher_heartbeat_stats();
				smsp.prefetcher_heartbeat_stats();
			fprintf(stderr,"\n");
		}
		
	}

	if(cache_on)
	{
		l1.final_stats(0);
		l2.final_stats(0);
	}
	if(prefetcher_on)
		//sp.prefetcher_final_stats();
		smsp.prefetcher_final_stats();

	if(prefetcher_on)
		//sp.prefetcher_destroy();
		smsp.prefetcher_destroy();

	return 0;
}
