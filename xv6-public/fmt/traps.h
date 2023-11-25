3300 // x86 trap and interrupt constants.
3301 
3302 // Processor-defined:
3303 #define T_DIVIDE         0      // divide error
3304 #define T_DEBUG          1      // debug exception
3305 #define T_NMI            2      // non-maskable interrupt
3306 #define T_BRKPT          3      // breakpoint
3307 #define T_OFLOW          4      // overflow
3308 #define T_BOUND          5      // bounds check
3309 #define T_ILLOP          6      // illegal opcode
3310 #define T_DEVICE         7      // device not available
3311 #define T_DBLFLT         8      // double fault
3312 // #define T_COPROC      9      // reserved (not used since 486)
3313 #define T_TSS           10      // invalid task switch segment
3314 #define T_SEGNP         11      // segment not present
3315 #define T_STACK         12      // stack exception
3316 #define T_GPFLT         13      // general protection fault
3317 #define T_PGFLT         14      // page fault
3318 // #define T_RES        15      // reserved
3319 #define T_FPERR         16      // floating point error
3320 #define T_ALIGN         17      // aligment check
3321 #define T_MCHK          18      // machine check
3322 #define T_SIMDERR       19      // SIMD floating point error
3323 
3324 // These are arbitrarily chosen, but with care not to overlap
3325 // processor defined exceptions or interrupt vectors.
3326 #define T_SYSCALL       64      // system call
3327 #define T_DEFAULT      500      // catchall
3328 
3329 #define T_IRQ0          32      // IRQ 0 corresponds to int T_IRQ
3330 
3331 #define IRQ_TIMER        0
3332 #define IRQ_KBD          1
3333 #define IRQ_COM1         4
3334 #define IRQ_IDE         14
3335 #define IRQ_ERROR       19
3336 #define IRQ_SPURIOUS    31
3337 
3338 
3339 
3340 
3341 
3342 
3343 
3344 
3345 
3346 
3347 
3348 
3349 
