#ifndef VARS_H
#define VARS_H

#define B 1
#define KB (1024*B)
#define MB (1024*KB)

#define L1D_A 4
#define L1D_B 64*B
#define L1D_C 32*KB

#define L2_TYPE 1 //1=LRU;2=DRRIP
#define L2_A 16
#define L2_B 64*B
#define L2_C 2*MB
#define RRPV_SIZE 2
#define THROTTLE 32
#define PSEL_SIZE 10

#define LINE_SIZE_LOG 6
#define REGION_SIZE_LOG 12
#define OFFSET_LOG (REGION_SIZE_LOG - LINE_SIZE_LOG)
#define MAX_OFFSET (1<<(OFFSET_LOG))

#define SMS_ACC 16
#define SMS_FIL 32
#define SMS_PHT (16*1024)
#define SMS_PHT_A 8
#define WSSC_HELP 1

#define PLT_S (2*1024)
#define PLT_A 8
#define SAT_A 16
#define WSSC_I (4*1024*1024)
#define THRESHOLD 0.25


#endif /* VARS_H */

