7200 // See MultiProcessor Specification Version 1.[14]
7201 
7202 struct mp {             // floating pointer
7203   uchar signature[4];           // "_MP_"
7204   void *physaddr;               // phys addr of MP config table
7205   uchar length;                 // 1
7206   uchar specrev;                // [14]
7207   uchar checksum;               // all bytes must add up to 0
7208   uchar type;                   // MP system config type
7209   uchar imcrp;
7210   uchar reserved[3];
7211 };
7212 
7213 struct mpconf {         // configuration table header
7214   uchar signature[4];           // "PCMP"
7215   ushort length;                // total table length
7216   uchar version;                // [14]
7217   uchar checksum;               // all bytes must add up to 0
7218   uchar product[20];            // product id
7219   uint *oemtable;               // OEM table pointer
7220   ushort oemlength;             // OEM table length
7221   ushort entry;                 // entry count
7222   uint *lapicaddr;              // address of local APIC
7223   ushort xlength;               // extended table length
7224   uchar xchecksum;              // extended table checksum
7225   uchar reserved;
7226 };
7227 
7228 struct mpproc {         // processor table entry
7229   uchar type;                   // entry type (0)
7230   uchar apicid;                 // local APIC id
7231   uchar version;                // local APIC verison
7232   uchar flags;                  // CPU flags
7233     #define MPBOOT 0x02           // This proc is the bootstrap processor.
7234   uchar signature[4];           // CPU signature
7235   uint feature;                 // feature flags from CPUID instruction
7236   uchar reserved[8];
7237 };
7238 
7239 struct mpioapic {       // I/O APIC table entry
7240   uchar type;                   // entry type (2)
7241   uchar apicno;                 // I/O APIC id
7242   uchar version;                // I/O APIC version
7243   uchar flags;                  // I/O APIC flags
7244   uint *addr;                  // I/O APIC address
7245 };
7246 
7247 
7248 
7249 
7250 // Table entry types
7251 #define MPPROC    0x00  // One per processor
7252 #define MPBUS     0x01  // One per bus
7253 #define MPIOAPIC  0x02  // One per I/O APIC
7254 #define MPIOINTR  0x03  // One per bus interrupt source
7255 #define MPLINTR   0x04  // One per system interrupt source
7256 
7257 
7258 
7259 
7260 
7261 
7262 
7263 
7264 
7265 
7266 
7267 
7268 
7269 
7270 
7271 
7272 
7273 
7274 
7275 
7276 
7277 
7278 
7279 
7280 
7281 
7282 
7283 
7284 
7285 
7286 
7287 
7288 
7289 
7290 
7291 
7292 
7293 
7294 
7295 
7296 
7297 
7298 
7299 
7300 // Blank page.
7301 
7302 
7303 
7304 
7305 
7306 
7307 
7308 
7309 
7310 
7311 
7312 
7313 
7314 
7315 
7316 
7317 
7318 
7319 
7320 
7321 
7322 
7323 
7324 
7325 
7326 
7327 
7328 
7329 
7330 
7331 
7332 
7333 
7334 
7335 
7336 
7337 
7338 
7339 
7340 
7341 
7342 
7343 
7344 
7345 
7346 
7347 
7348 
7349 
