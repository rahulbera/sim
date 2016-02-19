#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include <string.h>
#include <stdbool.h>
#include "prefetcher/prefetcher.h"

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
static unsigned int begin, end;
static unsigned int count; 
static char help[1000] = 
				"Supported knobs:\n"
				"--trace <file>\n"
				"	It's a mandatory knob. Sim will not run without a trace file.\n"
				"--trace-dump\n"
				"	Dumps memtrace in stderr. Default false\n"
				"--debug\n"
				"	Enables debugging mode. Dumps debug log in stderr. Default false\n"
				"--hearbeat <int>\n"
				"	Sets heartbeat interval to supplied value. Default 1,000,000\n"
				"--hide-heartbeat\n"
				"	Hides heart beat stats. Default false\n"
				"--begin <n>\n"
				"	Begins trace processing from here. Default 1\n"
				"--end <n>\n"
				"	Ends trace processing here. Default till end\n";

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

	prefetcher_init(debug);

	unsigned int pc, addr, prefAddr;
	bool isRead;
	int n;
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
		
		if (trace_dump) fprintf(stderr, "%*x %*x %*d P:%*x L:%*x LO:%*d\n", 7, pc, 8, addr, 1, isRead, 5, addr>>12, 7, addr>>6, 2, (addr>>6)&63);
		if (debug) fprintf(stderr, "[ No:%d ]\n%x %x %d\n", count+2, pc, addr, isRead);		
		prefetcher_operate(pc, addr, (&prefAddr));;

		if(!hide_heart_beat && (count%heart_beat)==0)
		{
			fprintf(stderr, "Processed: %*d ", 3, count/heart_beat);
			prefetcher_heartbeat_stats();
		}
		
	}

	fprintf(stdout, "[ Prefetcher ]\n");
	prefetcher_final_stats();

	prefetcher_destroy();

	return 0;
}
