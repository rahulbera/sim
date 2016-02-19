#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <zlib.h>
#include <stdbool.h>
#include <unordered_set>
#include <unordered_map>

#define HEART_BEAT 1000000
#define PAGE_SIZE_LOG 12
#define LINE_SIZE_LOG 6
#define ASSERT(cond,msg) do{if(!(cond)){fprintf(stderr,msg);exit(1);}}while(0)

using namespace std;

typedef struct entry
{
	unsigned int last_occurence;
	unordered_map<unsigned int,unsigned int> *rereference_map;
}entry_t;

unordered_set<unsigned int> page_map;
unordered_set<unsigned int> line_map;
unordered_set<unsigned int>::iterator p_map_it;
unordered_map<unsigned int, entry_t*> cache_line_map;
unordered_map<unsigned int, entry_t*>::iterator cl_map_it;
unordered_map<unsigned int, unsigned int>::iterator it;
unsigned int stat_totalMemAccess = 0;

gzFile gfp;
FILE *fp;


int main(int argc, char *argv[])
{
	

	ASSERT(argc==2,"Need <traceFile> as argument\n");

	fprintf(stderr, "Trace file: %s\n", argv[1]);
	gfp = gzopen(argv[1],"r");
	ASSERT(gfp!=Z_NULL,"Can't open file\n");
	fp = fopen("out.csv","w");
	ASSERT(fp!=NULL, "Cant't open output file\n");

	unsigned int pc, addr, page, line, gap;
	bool isRead;
	int n;
	while(gzread(gfp, (void*)(&pc), sizeof(unsigned int)) != 0)
	{
		n = gzread(gfp, (void*)(&addr), sizeof(unsigned int));
		ASSERT(n!=0, "Address reading failed!\n");
		n = gzread(gfp, (void*)(&isRead), sizeof(bool));
		ASSERT(n!=0, "isRead reading failed!\n");

		stat_totalMemAccess++;
		
		page = addr >> PAGE_SIZE_LOG;
		if(page_map.find(page) == page_map.end())
			page_map.insert(page);

		line = addr >> LINE_SIZE_LOG;
		if(line_map.find(line) == line_map.end())
			line_map.insert(line);

		/*cl_map_it = cache_line_map.find(line);
		if(cl_map_it == cache_line_map.end())
		{
			entry_t *e = (entry_t*)malloc(sizeof(entry_t));
			e->last_occurence = stat_totalMemAccess;
			e->rereference_map = new unordered_map<unsigned,unsigned>;
			cache_line_map.insert(pair<unsigned int, entry_t*>(line,e));
		}
		else
		{
			gap = stat_totalMemAccess - cl_map_it->second->last_occurence - 1;
			it = cl_map_it->second->rereference_map->find(gap);
			if(it == cl_map_it->second->rereference_map->end())
				cl_map_it->second->rereference_map->insert(pair<unsigned int, unsigned int>(gap,1));
			else
				it->second++;
			cl_map_it->second->last_occurence = stat_totalMemAccess;
		}*/

		if(stat_totalMemAccess % HEART_BEAT == 0)
			fprintf(stderr, "Processed: %d M\n", stat_totalMemAccess/HEART_BEAT);
	}

	/*unsigned int max = 0, mode = 0;
	for(cl_map_it = cache_line_map.begin(); cl_map_it != cache_line_map.end(); ++cl_map_it)
	{
		fprintf(fp, "%x, ", cl_map_it->first);
		max = 0;
		for(it = cl_map_it->second->rereference_map->begin(); it != cl_map_it->second->rereference_map->end(); ++it)
		{
			if(it->second > max)
			{
				max = it->second;
				mode = it->first;
			}
		}
		fprintf(fp, "%d\n", mode);
	}*/

	fprintf(stdout, "\nTotal Memory Access: %d\n", stat_totalMemAccess);
	fprintf(stdout, "Distinct pages: %lu\n", page_map.size());
	fprintf(stdout, "Distinct lines: %lu\n", line_map.size());

	return 0;
}