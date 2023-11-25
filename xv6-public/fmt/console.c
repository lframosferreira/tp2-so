8050 // Console input and output.
8051 // Input is from the keyboard or serial port.
8052 // Output is written to the screen and serial port.
8053 
8054 #include "types.h"
8055 #include "defs.h"
8056 #include "param.h"
8057 #include "traps.h"
8058 #include "spinlock.h"
8059 #include "sleeplock.h"
8060 #include "fs.h"
8061 #include "file.h"
8062 #include "memlayout.h"
8063 #include "mmu.h"
8064 #include "proc.h"
8065 #include "x86.h"
8066 
8067 static void consputc(int);
8068 
8069 static int panicked = 0;
8070 
8071 static struct {
8072   struct spinlock lock;
8073   int locking;
8074 } cons;
8075 
8076 static void
8077 printint(int xx, int base, int sign)
8078 {
8079   static char digits[] = "0123456789abcdef";
8080   char buf[16];
8081   int i;
8082   uint x;
8083 
8084   if(sign && (sign = xx < 0))
8085     x = -xx;
8086   else
8087     x = xx;
8088 
8089   i = 0;
8090   do{
8091     buf[i++] = digits[x % base];
8092   }while((x /= base) != 0);
8093 
8094   if(sign)
8095     buf[i++] = '-';
8096 
8097   while(--i >= 0)
8098     consputc(buf[i]);
8099 }
8100 
8101 
8102 
8103 
8104 
8105 
8106 
8107 
8108 
8109 
8110 
8111 
8112 
8113 
8114 
8115 
8116 
8117 
8118 
8119 
8120 
8121 
8122 
8123 
8124 
8125 
8126 
8127 
8128 
8129 
8130 
8131 
8132 
8133 
8134 
8135 
8136 
8137 
8138 
8139 
8140 
8141 
8142 
8143 
8144 
8145 
8146 
8147 
8148 
8149 
8150 // Print to the console. only understands %d, %x, %p, %s.
8151 void
8152 cprintf(char *fmt, ...)
8153 {
8154   int i, c, locking;
8155   uint *argp;
8156   char *s;
8157 
8158   locking = cons.locking;
8159   if(locking)
8160     acquire(&cons.lock);
8161 
8162   if (fmt == 0)
8163     panic("null fmt");
8164 
8165   argp = (uint*)(void*)(&fmt + 1);
8166   for(i = 0; (c = fmt[i] & 0xff) != 0; i++){
8167     if(c != '%'){
8168       consputc(c);
8169       continue;
8170     }
8171     c = fmt[++i] & 0xff;
8172     if(c == 0)
8173       break;
8174     switch(c){
8175     case 'd':
8176       printint(*argp++, 10, 1);
8177       break;
8178     case 'x':
8179     case 'p':
8180       printint(*argp++, 16, 0);
8181       break;
8182     case 's':
8183       if((s = (char*)*argp++) == 0)
8184         s = "(null)";
8185       for(; *s; s++)
8186         consputc(*s);
8187       break;
8188     case '%':
8189       consputc('%');
8190       break;
8191     default:
8192       // Print unknown % sequence to draw attention.
8193       consputc('%');
8194       consputc(c);
8195       break;
8196     }
8197   }
8198 
8199 
8200   if(locking)
8201     release(&cons.lock);
8202 }
8203 
8204 void
8205 panic(char *s)
8206 {
8207   int i;
8208   uint pcs[10];
8209 
8210   cli();
8211   cons.locking = 0;
8212   // use lapiccpunum so that we can call panic from mycpu()
8213   cprintf("lapicid %d: panic: ", lapicid());
8214   cprintf(s);
8215   cprintf("\n");
8216   getcallerpcs(&s, pcs);
8217   for(i=0; i<10; i++)
8218     cprintf(" %p", pcs[i]);
8219   panicked = 1; // freeze other CPU
8220   for(;;)
8221     ;
8222 }
8223 
8224 
8225 
8226 
8227 
8228 
8229 
8230 
8231 
8232 
8233 
8234 
8235 
8236 
8237 
8238 
8239 
8240 
8241 
8242 
8243 
8244 
8245 
8246 
8247 
8248 
8249 
8250 #define BACKSPACE 0x100
8251 #define CRTPORT 0x3d4
8252 static ushort *crt = (ushort*)P2V(0xb8000);  // CGA memory
8253 
8254 static void
8255 cgaputc(int c)
8256 {
8257   int pos;
8258 
8259   // Cursor position: col + 80*row.
8260   outb(CRTPORT, 14);
8261   pos = inb(CRTPORT+1) << 8;
8262   outb(CRTPORT, 15);
8263   pos |= inb(CRTPORT+1);
8264 
8265   if(c == '\n')
8266     pos += 80 - pos%80;
8267   else if(c == BACKSPACE){
8268     if(pos > 0) --pos;
8269   } else
8270     crt[pos++] = (c&0xff) | 0x0700;  // black on white
8271 
8272   if(pos < 0 || pos > 25*80)
8273     panic("pos under/overflow");
8274 
8275   if((pos/80) >= 24){  // Scroll up.
8276     memmove(crt, crt+80, sizeof(crt[0])*23*80);
8277     pos -= 80;
8278     memset(crt+pos, 0, sizeof(crt[0])*(24*80 - pos));
8279   }
8280 
8281   outb(CRTPORT, 14);
8282   outb(CRTPORT+1, pos>>8);
8283   outb(CRTPORT, 15);
8284   outb(CRTPORT+1, pos);
8285   crt[pos] = ' ' | 0x0700;
8286 }
8287 
8288 
8289 
8290 
8291 
8292 
8293 
8294 
8295 
8296 
8297 
8298 
8299 
8300 void
8301 consputc(int c)
8302 {
8303   if(panicked){
8304     cli();
8305     for(;;)
8306       ;
8307   }
8308 
8309   if(c == BACKSPACE){
8310     uartputc('\b'); uartputc(' '); uartputc('\b');
8311   } else
8312     uartputc(c);
8313   cgaputc(c);
8314 }
8315 
8316 #define INPUT_BUF 128
8317 struct {
8318   char buf[INPUT_BUF];
8319   uint r;  // Read index
8320   uint w;  // Write index
8321   uint e;  // Edit index
8322 } input;
8323 
8324 #define C(x)  ((x)-'@')  // Control-x
8325 
8326 void
8327 consoleintr(int (*getc)(void))
8328 {
8329   int c, doprocdump = 0;
8330 
8331   acquire(&cons.lock);
8332   while((c = getc()) >= 0){
8333     switch(c){
8334     case C('P'):  // Process listing.
8335       // procdump() locks cons.lock indirectly; invoke later
8336       doprocdump = 1;
8337       break;
8338     case C('U'):  // Kill line.
8339       while(input.e != input.w &&
8340             input.buf[(input.e-1) % INPUT_BUF] != '\n'){
8341         input.e--;
8342         consputc(BACKSPACE);
8343       }
8344       break;
8345     case C('H'): case '\x7f':  // Backspace
8346       if(input.e != input.w){
8347         input.e--;
8348         consputc(BACKSPACE);
8349       }
8350       break;
8351     default:
8352       if(c != 0 && input.e-input.r < INPUT_BUF){
8353         c = (c == '\r') ? '\n' : c;
8354         input.buf[input.e++ % INPUT_BUF] = c;
8355         consputc(c);
8356         if(c == '\n' || c == C('D') || input.e == input.r+INPUT_BUF){
8357           input.w = input.e;
8358           wakeup(&input.r);
8359         }
8360       }
8361       break;
8362     }
8363   }
8364   release(&cons.lock);
8365   if(doprocdump) {
8366     procdump();  // now call procdump() wo. cons.lock held
8367   }
8368 }
8369 
8370 int
8371 consoleread(struct inode *ip, char *dst, int n)
8372 {
8373   uint target;
8374   int c;
8375 
8376   iunlock(ip);
8377   target = n;
8378   acquire(&cons.lock);
8379   while(n > 0){
8380     while(input.r == input.w){
8381       if(myproc()->killed){
8382         release(&cons.lock);
8383         ilock(ip);
8384         return -1;
8385       }
8386       sleep(&input.r, &cons.lock);
8387     }
8388     c = input.buf[input.r++ % INPUT_BUF];
8389     if(c == C('D')){  // EOF
8390       if(n < target){
8391         // Save ^D for next time, to make sure
8392         // caller gets a 0-byte result.
8393         input.r--;
8394       }
8395       break;
8396     }
8397     *dst++ = c;
8398     --n;
8399     if(c == '\n')
8400       break;
8401   }
8402   release(&cons.lock);
8403   ilock(ip);
8404 
8405   return target - n;
8406 }
8407 
8408 int
8409 consolewrite(struct inode *ip, char *buf, int n)
8410 {
8411   int i;
8412 
8413   iunlock(ip);
8414   acquire(&cons.lock);
8415   for(i = 0; i < n; i++)
8416     consputc(buf[i] & 0xff);
8417   release(&cons.lock);
8418   ilock(ip);
8419 
8420   return n;
8421 }
8422 
8423 void
8424 consoleinit(void)
8425 {
8426   initlock(&cons.lock, "console");
8427 
8428   devsw[CONSOLE].write = consolewrite;
8429   devsw[CONSOLE].read = consoleread;
8430   cons.locking = 1;
8431 
8432   ioapicenable(IRQ_KBD, 0);
8433 }
8434 
8435 
8436 
8437 
8438 
8439 
8440 
8441 
8442 
8443 
8444 
8445 
8446 
8447 
8448 
8449 
