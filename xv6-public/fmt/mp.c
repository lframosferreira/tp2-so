7350 // Multiprocessor support
7351 // Search memory for MP description structures.
7352 // http://developer.intel.com/design/pentium/datashts/24201606.pdf
7353 
7354 #include "types.h"
7355 #include "defs.h"
7356 #include "param.h"
7357 #include "memlayout.h"
7358 #include "mp.h"
7359 #include "x86.h"
7360 #include "mmu.h"
7361 #include "proc.h"
7362 
7363 struct cpu cpus[NCPU];
7364 int ncpu;
7365 uchar ioapicid;
7366 
7367 static uchar
7368 sum(uchar *addr, int len)
7369 {
7370   int i, sum;
7371 
7372   sum = 0;
7373   for(i=0; i<len; i++)
7374     sum += addr[i];
7375   return sum;
7376 }
7377 
7378 // Look for an MP structure in the len bytes at addr.
7379 static struct mp*
7380 mpsearch1(uint a, int len)
7381 {
7382   uchar *e, *p, *addr;
7383 
7384   addr = P2V(a);
7385   e = addr+len;
7386   for(p = addr; p < e; p += sizeof(struct mp))
7387     if(memcmp(p, "_MP_", 4) == 0 && sum(p, sizeof(struct mp)) == 0)
7388       return (struct mp*)p;
7389   return 0;
7390 }
7391 
7392 
7393 
7394 
7395 
7396 
7397 
7398 
7399 
7400 // Search for the MP Floating Pointer Structure, which according to the
7401 // spec is in one of the following three locations:
7402 // 1) in the first KB of the EBDA;
7403 // 2) in the last KB of system base memory;
7404 // 3) in the BIOS ROM between 0xE0000 and 0xFFFFF.
7405 static struct mp*
7406 mpsearch(void)
7407 {
7408   uchar *bda;
7409   uint p;
7410   struct mp *mp;
7411 
7412   bda = (uchar *) P2V(0x400);
7413   if((p = ((bda[0x0F]<<8)| bda[0x0E]) << 4)){
7414     if((mp = mpsearch1(p, 1024)))
7415       return mp;
7416   } else {
7417     p = ((bda[0x14]<<8)|bda[0x13])*1024;
7418     if((mp = mpsearch1(p-1024, 1024)))
7419       return mp;
7420   }
7421   return mpsearch1(0xF0000, 0x10000);
7422 }
7423 
7424 // Search for an MP configuration table.  For now,
7425 // don't accept the default configurations (physaddr == 0).
7426 // Check for correct signature, calculate the checksum and,
7427 // if correct, check the version.
7428 // To do: check extended table checksum.
7429 static struct mpconf*
7430 mpconfig(struct mp **pmp)
7431 {
7432   struct mpconf *conf;
7433   struct mp *mp;
7434 
7435   if((mp = mpsearch()) == 0 || mp->physaddr == 0)
7436     return 0;
7437   conf = (struct mpconf*) P2V((uint) mp->physaddr);
7438   if(memcmp(conf, "PCMP", 4) != 0)
7439     return 0;
7440   if(conf->version != 1 && conf->version != 4)
7441     return 0;
7442   if(sum((uchar*)conf, conf->length) != 0)
7443     return 0;
7444   *pmp = mp;
7445   return conf;
7446 }
7447 
7448 
7449 
7450 void
7451 mpinit(void)
7452 {
7453   uchar *p, *e;
7454   int ismp;
7455   struct mp *mp;
7456   struct mpconf *conf;
7457   struct mpproc *proc;
7458   struct mpioapic *ioapic;
7459 
7460   if((conf = mpconfig(&mp)) == 0)
7461     panic("Expect to run on an SMP");
7462   ismp = 1;
7463   lapic = (uint*)conf->lapicaddr;
7464   for(p=(uchar*)(conf+1), e=(uchar*)conf+conf->length; p<e; ){
7465     switch(*p){
7466     case MPPROC:
7467       proc = (struct mpproc*)p;
7468       if(ncpu < NCPU) {
7469         cpus[ncpu].apicid = proc->apicid;  // apicid may differ from ncpu
7470         ncpu++;
7471       }
7472       p += sizeof(struct mpproc);
7473       continue;
7474     case MPIOAPIC:
7475       ioapic = (struct mpioapic*)p;
7476       ioapicid = ioapic->apicno;
7477       p += sizeof(struct mpioapic);
7478       continue;
7479     case MPBUS:
7480     case MPIOINTR:
7481     case MPLINTR:
7482       p += 8;
7483       continue;
7484     default:
7485       ismp = 0;
7486       break;
7487     }
7488   }
7489   if(!ismp)
7490     panic("Didn't find a suitable machine");
7491 
7492   if(mp->imcrp){
7493     // Bochs doesn't support IMCR, so this doesn't run on Bochs.
7494     // But it would on real hardware.
7495     outb(0x22, 0x70);   // Select IMCR
7496     outb(0x23, inb(0x23) | 1);  // Mask external interrupts.
7497   }
7498 }
7499 
