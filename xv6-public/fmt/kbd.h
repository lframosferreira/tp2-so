7850 // PC keyboard interface constants
7851 
7852 #define KBSTATP         0x64    // kbd controller status port(I)
7853 #define KBS_DIB         0x01    // kbd data in buffer
7854 #define KBDATAP         0x60    // kbd data port(I)
7855 
7856 #define NO              0
7857 
7858 #define SHIFT           (1<<0)
7859 #define CTL             (1<<1)
7860 #define ALT             (1<<2)
7861 
7862 #define CAPSLOCK        (1<<3)
7863 #define NUMLOCK         (1<<4)
7864 #define SCROLLLOCK      (1<<5)
7865 
7866 #define E0ESC           (1<<6)
7867 
7868 // Special keycodes
7869 #define KEY_HOME        0xE0
7870 #define KEY_END         0xE1
7871 #define KEY_UP          0xE2
7872 #define KEY_DN          0xE3
7873 #define KEY_LF          0xE4
7874 #define KEY_RT          0xE5
7875 #define KEY_PGUP        0xE6
7876 #define KEY_PGDN        0xE7
7877 #define KEY_INS         0xE8
7878 #define KEY_DEL         0xE9
7879 
7880 // C('A') == Control-A
7881 #define C(x) (x - '@')
7882 
7883 static uchar shiftcode[256] =
7884 {
7885   [0x1D] CTL,
7886   [0x2A] SHIFT,
7887   [0x36] SHIFT,
7888   [0x38] ALT,
7889   [0x9D] CTL,
7890   [0xB8] ALT
7891 };
7892 
7893 static uchar togglecode[256] =
7894 {
7895   [0x3A] CAPSLOCK,
7896   [0x45] NUMLOCK,
7897   [0x46] SCROLLLOCK
7898 };
7899 
7900 static uchar normalmap[256] =
7901 {
7902   NO,   0x1B, '1',  '2',  '3',  '4',  '5',  '6',  // 0x00
7903   '7',  '8',  '9',  '0',  '-',  '=',  '\b', '\t',
7904   'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',  // 0x10
7905   'o',  'p',  '[',  ']',  '\n', NO,   'a',  's',
7906   'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';',  // 0x20
7907   '\'', '`',  NO,   '\\', 'z',  'x',  'c',  'v',
7908   'b',  'n',  'm',  ',',  '.',  '/',  NO,   '*',  // 0x30
7909   NO,   ' ',  NO,   NO,   NO,   NO,   NO,   NO,
7910   NO,   NO,   NO,   NO,   NO,   NO,   NO,   '7',  // 0x40
7911   '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',
7912   '2',  '3',  '0',  '.',  NO,   NO,   NO,   NO,   // 0x50
7913   [0x9C] '\n',      // KP_Enter
7914   [0xB5] '/',       // KP_Div
7915   [0xC8] KEY_UP,    [0xD0] KEY_DN,
7916   [0xC9] KEY_PGUP,  [0xD1] KEY_PGDN,
7917   [0xCB] KEY_LF,    [0xCD] KEY_RT,
7918   [0x97] KEY_HOME,  [0xCF] KEY_END,
7919   [0xD2] KEY_INS,   [0xD3] KEY_DEL
7920 };
7921 
7922 static uchar shiftmap[256] =
7923 {
7924   NO,   033,  '!',  '@',  '#',  '$',  '%',  '^',  // 0x00
7925   '&',  '*',  '(',  ')',  '_',  '+',  '\b', '\t',
7926   'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',  // 0x10
7927   'O',  'P',  '{',  '}',  '\n', NO,   'A',  'S',
7928   'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',  // 0x20
7929   '"',  '~',  NO,   '|',  'Z',  'X',  'C',  'V',
7930   'B',  'N',  'M',  '<',  '>',  '?',  NO,   '*',  // 0x30
7931   NO,   ' ',  NO,   NO,   NO,   NO,   NO,   NO,
7932   NO,   NO,   NO,   NO,   NO,   NO,   NO,   '7',  // 0x40
7933   '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',
7934   '2',  '3',  '0',  '.',  NO,   NO,   NO,   NO,   // 0x50
7935   [0x9C] '\n',      // KP_Enter
7936   [0xB5] '/',       // KP_Div
7937   [0xC8] KEY_UP,    [0xD0] KEY_DN,
7938   [0xC9] KEY_PGUP,  [0xD1] KEY_PGDN,
7939   [0xCB] KEY_LF,    [0xCD] KEY_RT,
7940   [0x97] KEY_HOME,  [0xCF] KEY_END,
7941   [0xD2] KEY_INS,   [0xD3] KEY_DEL
7942 };
7943 
7944 
7945 
7946 
7947 
7948 
7949 
7950 static uchar ctlmap[256] =
7951 {
7952   NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
7953   NO,      NO,      NO,      NO,      NO,      NO,      NO,      NO,
7954   C('Q'),  C('W'),  C('E'),  C('R'),  C('T'),  C('Y'),  C('U'),  C('I'),
7955   C('O'),  C('P'),  NO,      NO,      '\r',    NO,      C('A'),  C('S'),
7956   C('D'),  C('F'),  C('G'),  C('H'),  C('J'),  C('K'),  C('L'),  NO,
7957   NO,      NO,      NO,      C('\\'), C('Z'),  C('X'),  C('C'),  C('V'),
7958   C('B'),  C('N'),  C('M'),  NO,      NO,      C('/'),  NO,      NO,
7959   [0x9C] '\r',      // KP_Enter
7960   [0xB5] C('/'),    // KP_Div
7961   [0xC8] KEY_UP,    [0xD0] KEY_DN,
7962   [0xC9] KEY_PGUP,  [0xD1] KEY_PGDN,
7963   [0xCB] KEY_LF,    [0xCD] KEY_RT,
7964   [0x97] KEY_HOME,  [0xCF] KEY_END,
7965   [0xD2] KEY_INS,   [0xD3] KEY_DEL
7966 };
7967 
7968 
7969 
7970 
7971 
7972 
7973 
7974 
7975 
7976 
7977 
7978 
7979 
7980 
7981 
7982 
7983 
7984 
7985 
7986 
7987 
7988 
7989 
7990 
7991 
7992 
7993 
7994 
7995 
7996 
7997 
7998 
7999 
