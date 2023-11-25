3200 // Physical memory allocator, intended to allocate
3201 // memory for user processes, kernel stacks, page table pages,
3202 // and pipe buffers. Allocates 4096-byte pages.
3203 
3204 #include "types.h"
3205 #include "defs.h"
3206 #include "param.h"
3207 #include "memlayout.h"
3208 #include "mmu.h"
3209 #include "spinlock.h"
3210 
3211 void freerange(void *vstart, void *vend);
3212 extern char end[]; // first address after kernel loaded from ELF file
3213                    // defined by the kernel linker script in kernel.ld
3214 
3215 struct run {
3216   struct run *next;
3217 };
3218 
3219 struct {
3220   struct spinlock lock;
3221   int use_lock;
3222   struct run *freelist;
3223 } kmem;
3224 
3225 // Initialization happens in two phases.
3226 // 1. main() calls kinit1() while still using entrypgdir to place just
3227 // the pages mapped by entrypgdir on free list.
3228 // 2. main() calls kinit2() with the rest of the physical pages
3229 // after installing a full page table that maps them on all cores.
3230 void
3231 kinit1(void *vstart, void *vend)
3232 {
3233   initlock(&kmem.lock, "kmem");
3234   kmem.use_lock = 0;
3235   freerange(vstart, vend);
3236 }
3237 
3238 void
3239 kinit2(void *vstart, void *vend)
3240 {
3241   freerange(vstart, vend);
3242   kmem.use_lock = 1;
3243 }
3244 
3245 
3246 
3247 
3248 
3249 
3250 void
3251 freerange(void *vstart, void *vend)
3252 {
3253   char *p;
3254   p = (char*)PGROUNDUP((uint)vstart);
3255   for(; p + PGSIZE <= (char*)vend; p += PGSIZE)
3256     kfree(p);
3257 }
3258 
3259 // Free the page of physical memory pointed at by v,
3260 // which normally should have been returned by a
3261 // call to kalloc().  (The exception is when
3262 // initializing the allocator; see kinit above.)
3263 void
3264 kfree(char *v)
3265 {
3266   struct run *r;
3267 
3268   if((uint)v % PGSIZE || v < end || V2P(v) >= PHYSTOP)
3269     panic("kfree");
3270 
3271   // Fill with junk to catch dangling refs.
3272   memset(v, 1, PGSIZE);
3273 
3274   if(kmem.use_lock)
3275     acquire(&kmem.lock);
3276   r = (struct run*)v;
3277   r->next = kmem.freelist;
3278   kmem.freelist = r;
3279   if(kmem.use_lock)
3280     release(&kmem.lock);
3281 }
3282 
3283 // Allocate one 4096-byte page of physical memory.
3284 // Returns a pointer that the kernel can use.
3285 // Returns 0 if the memory cannot be allocated.
3286 char*
3287 kalloc(void)
3288 {
3289   struct run *r;
3290 
3291   if(kmem.use_lock)
3292     acquire(&kmem.lock);
3293   r = kmem.freelist;
3294   if(r)
3295     kmem.freelist = r->next;
3296   if(kmem.use_lock)
3297     release(&kmem.lock);
3298   return (char*)r;
3299 }
