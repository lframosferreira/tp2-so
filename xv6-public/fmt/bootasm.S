9250 #include "asm.h"
9251 #include "memlayout.h"
9252 #include "mmu.h"
9253 
9254 # Start the first CPU: switch to 32-bit protected mode, jump into C.
9255 # The BIOS loads this code from the first sector of the hard disk into
9256 # memory at physical address 0x7c00 and starts executing in real mode
9257 # with %cs=0 %ip=7c00.
9258 
9259 .code16                       # Assemble for 16-bit mode
9260 .globl start
9261 start:
9262   cli                         # BIOS enabled interrupts; disable
9263 
9264   # Zero data segment registers DS, ES, and SS.
9265   xorw    %ax,%ax             # Set %ax to zero
9266   movw    %ax,%ds             # -> Data Segment
9267   movw    %ax,%es             # -> Extra Segment
9268   movw    %ax,%ss             # -> Stack Segment
9269 
9270   # Physical address line A20 is tied to zero so that the first PCs
9271   # with 2 MB would run software that assumed 1 MB.  Undo that.
9272 seta20.1:
9273   inb     $0x64,%al               # Wait for not busy
9274   testb   $0x2,%al
9275   jnz     seta20.1
9276 
9277   movb    $0xd1,%al               # 0xd1 -> port 0x64
9278   outb    %al,$0x64
9279 
9280 seta20.2:
9281   inb     $0x64,%al               # Wait for not busy
9282   testb   $0x2,%al
9283   jnz     seta20.2
9284 
9285   movb    $0xdf,%al               # 0xdf -> port 0x60
9286   outb    %al,$0x60
9287 
9288   # Switch from real to protected mode.  Use a bootstrap GDT that makes
9289   # virtual addresses map directly to physical addresses so that the
9290   # effective memory map doesn't change during the transition.
9291   lgdt    gdtdesc
9292   movl    %cr0, %eax
9293   orl     $CR0_PE, %eax
9294   movl    %eax, %cr0
9295 
9296 
9297 
9298 
9299 
9300   # Complete the transition to 32-bit protected mode by using a long jmp
9301   # to reload %cs and %eip.  The segment descriptors are set up with no
9302   # translation, so that the mapping is still the identity mapping.
9303   ljmp    $(SEG_KCODE<<3), $start32
9304 
9305 .code32  # Tell assembler to generate 32-bit code now.
9306 start32:
9307   # Set up the protected-mode data segment registers
9308   movw    $(SEG_KDATA<<3), %ax    # Our data segment selector
9309   movw    %ax, %ds                # -> DS: Data Segment
9310   movw    %ax, %es                # -> ES: Extra Segment
9311   movw    %ax, %ss                # -> SS: Stack Segment
9312   movw    $0, %ax                 # Zero segments not ready for use
9313   movw    %ax, %fs                # -> FS
9314   movw    %ax, %gs                # -> GS
9315 
9316   # Set up the stack pointer and call into C.
9317   movl    $start, %esp
9318   call    bootmain
9319 
9320   # If bootmain returns (it shouldn't), trigger a Bochs
9321   # breakpoint if running under Bochs, then loop.
9322   movw    $0x8a00, %ax            # 0x8a00 -> port 0x8a00
9323   movw    %ax, %dx
9324   outw    %ax, %dx
9325   movw    $0x8ae0, %ax            # 0x8ae0 -> port 0x8a00
9326   outw    %ax, %dx
9327 spin:
9328   jmp     spin
9329 
9330 # Bootstrap GDT
9331 .p2align 2                                # force 4 byte alignment
9332 gdt:
9333   SEG_NULLASM                             # null seg
9334   SEG_ASM(STA_X|STA_R, 0x0, 0xffffffff)   # code seg
9335   SEG_ASM(STA_W, 0x0, 0xffffffff)         # data seg
9336 
9337 gdtdesc:
9338   .word   (gdtdesc - gdt - 1)             # sizeof(gdt) - 1
9339   .long   gdt                             # address gdt
9340 
9341 
9342 
9343 
9344 
9345 
9346 
9347 
9348 
9349 
