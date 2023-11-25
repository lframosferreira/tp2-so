0150 #define NPROC        64  // maximum number of processes
0151 #define KSTACKSIZE 4096  // size of per-process kernel stack
0152 #define NCPU          8  // maximum number of CPUs
0153 #define NOFILE       16  // open files per process
0154 #define NFILE       100  // open files per system
0155 #define NINODE       50  // maximum number of active i-nodes
0156 #define NDEV         10  // maximum major device number
0157 #define ROOTDEV       1  // device number of file system root disk
0158 #define MAXARG       32  // max exec arguments
0159 #define MAXOPBLOCKS  10  // max # of blocks any FS op writes
0160 #define LOGSIZE      (MAXOPBLOCKS*3)  // max data blocks in on-disk log
0161 #define NBUF         (MAXOPBLOCKS*3)  // size of disk block cache
0162 #define FSSIZE       1000  // size of file system in blocks
0163 
0164 #define INTERV 5 // interval between preemptions
0165 #define P1TO2 200 //Promove um processo da fila 1 para fila 2 se tempo de espera maior que 1TO2 ticks
0166 #define P2TO3 100 //Promove um processo da fila 2 para fila 3 se tempo de espera maior que 2TO3 ticks
0167 
0168 
0169 
0170 
0171 
0172 
0173 
0174 
0175 
0176 
0177 
0178 
0179 
0180 
0181 
0182 
0183 
0184 
0185 
0186 
0187 
0188 
0189 
0190 
0191 
0192 
0193 
0194 
0195 
0196 
0197 
0198 
0199 
