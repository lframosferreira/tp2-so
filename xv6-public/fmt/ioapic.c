7750 // The I/O APIC manages hardware interrupts for an SMP system.
7751 // http://www.intel.com/design/chipsets/datashts/29056601.pdf
7752 // See also picirq.c.
7753 
7754 #include "types.h"
7755 #include "defs.h"
7756 #include "traps.h"
7757 
7758 #define IOAPIC  0xFEC00000   // Default physical address of IO APIC
7759 
7760 #define REG_ID     0x00  // Register index: ID
7761 #define REG_VER    0x01  // Register index: version
7762 #define REG_TABLE  0x10  // Redirection table base
7763 
7764 // The redirection table starts at REG_TABLE and uses
7765 // two registers to configure each interrupt.
7766 // The first (low) register in a pair contains configuration bits.
7767 // The second (high) register contains a bitmask telling which
7768 // CPUs can serve that interrupt.
7769 #define INT_DISABLED   0x00010000  // Interrupt disabled
7770 #define INT_LEVEL      0x00008000  // Level-triggered (vs edge-)
7771 #define INT_ACTIVELOW  0x00002000  // Active low (vs high)
7772 #define INT_LOGICAL    0x00000800  // Destination is CPU id (vs APIC ID)
7773 
7774 volatile struct ioapic *ioapic;
7775 
7776 // IO APIC MMIO structure: write reg, then read or write data.
7777 struct ioapic {
7778   uint reg;
7779   uint pad[3];
7780   uint data;
7781 };
7782 
7783 static uint
7784 ioapicread(int reg)
7785 {
7786   ioapic->reg = reg;
7787   return ioapic->data;
7788 }
7789 
7790 static void
7791 ioapicwrite(int reg, uint data)
7792 {
7793   ioapic->reg = reg;
7794   ioapic->data = data;
7795 }
7796 
7797 
7798 
7799 
7800 void
7801 ioapicinit(void)
7802 {
7803   int i, id, maxintr;
7804 
7805   ioapic = (volatile struct ioapic*)IOAPIC;
7806   maxintr = (ioapicread(REG_VER) >> 16) & 0xFF;
7807   id = ioapicread(REG_ID) >> 24;
7808   if(id != ioapicid)
7809     cprintf("ioapicinit: id isn't equal to ioapicid; not a MP\n");
7810 
7811   // Mark all interrupts edge-triggered, active high, disabled,
7812   // and not routed to any CPUs.
7813   for(i = 0; i <= maxintr; i++){
7814     ioapicwrite(REG_TABLE+2*i, INT_DISABLED | (T_IRQ0 + i));
7815     ioapicwrite(REG_TABLE+2*i+1, 0);
7816   }
7817 }
7818 
7819 void
7820 ioapicenable(int irq, int cpunum)
7821 {
7822   // Mark interrupt edge-triggered, active high,
7823   // enabled, and routed to the given cpunum,
7824   // which happens to be that cpu's APIC ID.
7825   ioapicwrite(REG_TABLE+2*irq, T_IRQ0 + irq);
7826   ioapicwrite(REG_TABLE+2*irq+1, cpunum << 24);
7827 }
7828 
7829 
7830 
7831 
7832 
7833 
7834 
7835 
7836 
7837 
7838 
7839 
7840 
7841 
7842 
7843 
7844 
7845 
7846 
7847 
7848 
7849 
