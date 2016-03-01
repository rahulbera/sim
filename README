This is a simple simulator which models a two level cache hierarchy with prefetcher support.

*
*	How to compile
*
make release 	- for release build
make debug		- for debug build

*
*	How to run
*
Currently sim supports these knobs:

--trace <file>
	It's a mandatory knob. Sim will not run without a trace file.
--hearbeat <int>
	Sets heartbeat interval to supplied value. Default 1,000,000
--begin <n>
	Begins trace processing from here. Default 1
--end <n>
	Ends trace processing here. Default till end.
--trace-dump
	Dumps memtrace in stderr. Not compatible with other knobs except trace.
	Disables cache and prefetcher by default. Hides heartbeat.
--l1trace-dump
	Dumps l1miss memtrace in stderr. Don't use it with other trace dumps.
	Enables cache by default. Hides heartbeat.
--l2trace-dump
	Dumps l2miss memtrace in stderr. Don't use it with other trace dumps.
	Enables cache by default. Hides heartbeat.
--[l1|l2]verbose <n>
	Cache verbose level, from 0-3. Default 0.
--hide-heartbeat
	Hides heart beat stats.
--no-prefetch
	Disables prefetcher.
--no-cache
	Disables cache hierarchy.

You need a trace file to run with this simulator. One sample trace is supplied 
in trace directory. You can also generate more traces using the pintool.

*
*	Implementation
*
Currently sim supports Multi-stream stride prefetcher [Nesbit et.al, HPCA'04] and 
Spatial Memory Streaming [Somogyi et.al, ISCA'06]. For cache model, we used LRU policy.