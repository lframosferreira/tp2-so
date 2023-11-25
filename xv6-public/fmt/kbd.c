8000 #include "types.h"
8001 #include "x86.h"
8002 #include "defs.h"
8003 #include "kbd.h"
8004 
8005 int
8006 kbdgetc(void)
8007 {
8008   static uint shift;
8009   static uchar *charcode[4] = {
8010     normalmap, shiftmap, ctlmap, ctlmap
8011   };
8012   uint st, data, c;
8013 
8014   st = inb(KBSTATP);
8015   if((st & KBS_DIB) == 0)
8016     return -1;
8017   data = inb(KBDATAP);
8018 
8019   if(data == 0xE0){
8020     shift |= E0ESC;
8021     return 0;
8022   } else if(data & 0x80){
8023     // Key released
8024     data = (shift & E0ESC ? data : data & 0x7F);
8025     shift &= ~(shiftcode[data] | E0ESC);
8026     return 0;
8027   } else if(shift & E0ESC){
8028     // Last character was an E0 escape; or with 0x80
8029     data |= 0x80;
8030     shift &= ~E0ESC;
8031   }
8032 
8033   shift |= shiftcode[data];
8034   shift ^= togglecode[data];
8035   c = charcode[shift & (CTL | SHIFT)][data];
8036   if(shift & CAPSLOCK){
8037     if('a' <= c && c <= 'z')
8038       c += 'A' - 'a';
8039     else if('A' <= c && c <= 'Z')
8040       c += 'a' - 'A';
8041   }
8042   return c;
8043 }
8044 
8045 void
8046 kbdintr(void)
8047 {
8048   consoleintr(kbdgetc);
8049 }
