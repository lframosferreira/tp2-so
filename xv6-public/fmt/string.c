7050 #include "types.h"
7051 #include "x86.h"
7052 
7053 void*
7054 memset(void *dst, int c, uint n)
7055 {
7056   if ((int)dst%4 == 0 && n%4 == 0){
7057     c &= 0xFF;
7058     stosl(dst, (c<<24)|(c<<16)|(c<<8)|c, n/4);
7059   } else
7060     stosb(dst, c, n);
7061   return dst;
7062 }
7063 
7064 int
7065 memcmp(const void *v1, const void *v2, uint n)
7066 {
7067   const uchar *s1, *s2;
7068 
7069   s1 = v1;
7070   s2 = v2;
7071   while(n-- > 0){
7072     if(*s1 != *s2)
7073       return *s1 - *s2;
7074     s1++, s2++;
7075   }
7076 
7077   return 0;
7078 }
7079 
7080 void*
7081 memmove(void *dst, const void *src, uint n)
7082 {
7083   const char *s;
7084   char *d;
7085 
7086   s = src;
7087   d = dst;
7088   if(s < d && s + n > d){
7089     s += n;
7090     d += n;
7091     while(n-- > 0)
7092       *--d = *--s;
7093   } else
7094     while(n-- > 0)
7095       *d++ = *s++;
7096 
7097   return dst;
7098 }
7099 
7100 // memcpy exists to placate GCC.  Use memmove.
7101 void*
7102 memcpy(void *dst, const void *src, uint n)
7103 {
7104   return memmove(dst, src, n);
7105 }
7106 
7107 int
7108 strncmp(const char *p, const char *q, uint n)
7109 {
7110   while(n > 0 && *p && *p == *q)
7111     n--, p++, q++;
7112   if(n == 0)
7113     return 0;
7114   return (uchar)*p - (uchar)*q;
7115 }
7116 
7117 char*
7118 strncpy(char *s, const char *t, int n)
7119 {
7120   char *os;
7121 
7122   os = s;
7123   while(n-- > 0 && (*s++ = *t++) != 0)
7124     ;
7125   while(n-- > 0)
7126     *s++ = 0;
7127   return os;
7128 }
7129 
7130 // Like strncpy but guaranteed to NUL-terminate.
7131 char*
7132 safestrcpy(char *s, const char *t, int n)
7133 {
7134   char *os;
7135 
7136   os = s;
7137   if(n <= 0)
7138     return os;
7139   while(--n > 0 && (*s++ = *t++) != 0)
7140     ;
7141   *s = 0;
7142   return os;
7143 }
7144 
7145 
7146 
7147 
7148 
7149 
7150 int
7151 strlen(const char *s)
7152 {
7153   int n;
7154 
7155   for(n = 0; s[n]; n++)
7156     ;
7157   return n;
7158 }
7159 
7160 
7161 
7162 
7163 
7164 
7165 
7166 
7167 
7168 
7169 
7170 
7171 
7172 
7173 
7174 
7175 
7176 
7177 
7178 
7179 
7180 
7181 
7182 
7183 
7184 
7185 
7186 
7187 
7188 
7189 
7190 
7191 
7192 
7193 
7194 
7195 
7196 
7197 
7198 
7199 
