8700 // Shell.
8701 
8702 #include "types.h"
8703 #include "user.h"
8704 #include "fcntl.h"
8705 
8706 // Parsed command representation
8707 #define EXEC  1
8708 #define REDIR 2
8709 #define PIPE  3
8710 #define LIST  4
8711 #define BACK  5
8712 
8713 #define MAXARGS 10
8714 
8715 struct cmd {
8716   int type;
8717 };
8718 
8719 struct execcmd {
8720   int type;
8721   char *argv[MAXARGS];
8722   char *eargv[MAXARGS];
8723 };
8724 
8725 struct redircmd {
8726   int type;
8727   struct cmd *cmd;
8728   char *file;
8729   char *efile;
8730   int mode;
8731   int fd;
8732 };
8733 
8734 struct pipecmd {
8735   int type;
8736   struct cmd *left;
8737   struct cmd *right;
8738 };
8739 
8740 struct listcmd {
8741   int type;
8742   struct cmd *left;
8743   struct cmd *right;
8744 };
8745 
8746 struct backcmd {
8747   int type;
8748   struct cmd *cmd;
8749 };
8750 int fork1(void);  // Fork but panics on failure.
8751 void panic(char*);
8752 struct cmd *parsecmd(char*);
8753 
8754 // Execute cmd.  Never returns.
8755 void
8756 runcmd(struct cmd *cmd)
8757 {
8758   int p[2];
8759   struct backcmd *bcmd;
8760   struct execcmd *ecmd;
8761   struct listcmd *lcmd;
8762   struct pipecmd *pcmd;
8763   struct redircmd *rcmd;
8764 
8765   if(cmd == 0)
8766     exit();
8767 
8768   switch(cmd->type){
8769   default:
8770     panic("runcmd");
8771 
8772   case EXEC:
8773     ecmd = (struct execcmd*)cmd;
8774     if(ecmd->argv[0] == 0)
8775       exit();
8776     exec(ecmd->argv[0], ecmd->argv);
8777     printf(2, "exec %s failed\n", ecmd->argv[0]);
8778     break;
8779 
8780   case REDIR:
8781     rcmd = (struct redircmd*)cmd;
8782     close(rcmd->fd);
8783     if(open(rcmd->file, rcmd->mode) < 0){
8784       printf(2, "open %s failed\n", rcmd->file);
8785       exit();
8786     }
8787     runcmd(rcmd->cmd);
8788     break;
8789 
8790   case LIST:
8791     lcmd = (struct listcmd*)cmd;
8792     if(fork1() == 0)
8793       runcmd(lcmd->left);
8794     wait();
8795     runcmd(lcmd->right);
8796     break;
8797 
8798 
8799 
8800   case PIPE:
8801     pcmd = (struct pipecmd*)cmd;
8802     if(pipe(p) < 0)
8803       panic("pipe");
8804     if(fork1() == 0){
8805       close(1);
8806       dup(p[1]);
8807       close(p[0]);
8808       close(p[1]);
8809       runcmd(pcmd->left);
8810     }
8811     if(fork1() == 0){
8812       close(0);
8813       dup(p[0]);
8814       close(p[0]);
8815       close(p[1]);
8816       runcmd(pcmd->right);
8817     }
8818     close(p[0]);
8819     close(p[1]);
8820     wait();
8821     wait();
8822     break;
8823 
8824   case BACK:
8825     bcmd = (struct backcmd*)cmd;
8826     if(fork1() == 0)
8827       runcmd(bcmd->cmd);
8828     break;
8829   }
8830   exit();
8831 }
8832 
8833 int
8834 getcmd(char *buf, int nbuf)
8835 {
8836   printf(2, "$ ");
8837   memset(buf, 0, nbuf);
8838   gets(buf, nbuf);
8839   if(buf[0] == 0) // EOF
8840     return -1;
8841   return 0;
8842 }
8843 
8844 
8845 
8846 
8847 
8848 
8849 
8850 int
8851 main(void)
8852 {
8853   static char buf[100];
8854   int fd;
8855 
8856   // Ensure that three file descriptors are open.
8857   while((fd = open("console", O_RDWR)) >= 0){
8858     if(fd >= 3){
8859       close(fd);
8860       break;
8861     }
8862   }
8863 
8864   // Read and run input commands.
8865   while(getcmd(buf, sizeof(buf)) >= 0){
8866     if(buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' '){
8867       // Chdir must be called by the parent, not the child.
8868       buf[strlen(buf)-1] = 0;  // chop \n
8869       if(chdir(buf+3) < 0)
8870         printf(2, "cannot cd %s\n", buf+3);
8871       continue;
8872     }
8873     if(fork1() == 0)
8874       runcmd(parsecmd(buf));
8875     wait();
8876   }
8877   exit();
8878 }
8879 
8880 void
8881 panic(char *s)
8882 {
8883   printf(2, "%s\n", s);
8884   exit();
8885 }
8886 
8887 int
8888 fork1(void)
8889 {
8890   int pid;
8891 
8892   pid = fork();
8893   if(pid == -1)
8894     panic("fork");
8895   return pid;
8896 }
8897 
8898 
8899 
8900 // Constructors
8901 
8902 struct cmd*
8903 execcmd(void)
8904 {
8905   struct execcmd *cmd;
8906 
8907   cmd = malloc(sizeof(*cmd));
8908   memset(cmd, 0, sizeof(*cmd));
8909   cmd->type = EXEC;
8910   return (struct cmd*)cmd;
8911 }
8912 
8913 struct cmd*
8914 redircmd(struct cmd *subcmd, char *file, char *efile, int mode, int fd)
8915 {
8916   struct redircmd *cmd;
8917 
8918   cmd = malloc(sizeof(*cmd));
8919   memset(cmd, 0, sizeof(*cmd));
8920   cmd->type = REDIR;
8921   cmd->cmd = subcmd;
8922   cmd->file = file;
8923   cmd->efile = efile;
8924   cmd->mode = mode;
8925   cmd->fd = fd;
8926   return (struct cmd*)cmd;
8927 }
8928 
8929 struct cmd*
8930 pipecmd(struct cmd *left, struct cmd *right)
8931 {
8932   struct pipecmd *cmd;
8933 
8934   cmd = malloc(sizeof(*cmd));
8935   memset(cmd, 0, sizeof(*cmd));
8936   cmd->type = PIPE;
8937   cmd->left = left;
8938   cmd->right = right;
8939   return (struct cmd*)cmd;
8940 }
8941 
8942 
8943 
8944 
8945 
8946 
8947 
8948 
8949 
8950 struct cmd*
8951 listcmd(struct cmd *left, struct cmd *right)
8952 {
8953   struct listcmd *cmd;
8954 
8955   cmd = malloc(sizeof(*cmd));
8956   memset(cmd, 0, sizeof(*cmd));
8957   cmd->type = LIST;
8958   cmd->left = left;
8959   cmd->right = right;
8960   return (struct cmd*)cmd;
8961 }
8962 
8963 struct cmd*
8964 backcmd(struct cmd *subcmd)
8965 {
8966   struct backcmd *cmd;
8967 
8968   cmd = malloc(sizeof(*cmd));
8969   memset(cmd, 0, sizeof(*cmd));
8970   cmd->type = BACK;
8971   cmd->cmd = subcmd;
8972   return (struct cmd*)cmd;
8973 }
8974 
8975 
8976 
8977 
8978 
8979 
8980 
8981 
8982 
8983 
8984 
8985 
8986 
8987 
8988 
8989 
8990 
8991 
8992 
8993 
8994 
8995 
8996 
8997 
8998 
8999 
9000 // Parsing
9001 
9002 char whitespace[] = " \t\r\n\v";
9003 char symbols[] = "<|>&;()";
9004 
9005 int
9006 gettoken(char **ps, char *es, char **q, char **eq)
9007 {
9008   char *s;
9009   int ret;
9010 
9011   s = *ps;
9012   while(s < es && strchr(whitespace, *s))
9013     s++;
9014   if(q)
9015     *q = s;
9016   ret = *s;
9017   switch(*s){
9018   case 0:
9019     break;
9020   case '|':
9021   case '(':
9022   case ')':
9023   case ';':
9024   case '&':
9025   case '<':
9026     s++;
9027     break;
9028   case '>':
9029     s++;
9030     if(*s == '>'){
9031       ret = '+';
9032       s++;
9033     }
9034     break;
9035   default:
9036     ret = 'a';
9037     while(s < es && !strchr(whitespace, *s) && !strchr(symbols, *s))
9038       s++;
9039     break;
9040   }
9041   if(eq)
9042     *eq = s;
9043 
9044   while(s < es && strchr(whitespace, *s))
9045     s++;
9046   *ps = s;
9047   return ret;
9048 }
9049 
9050 int
9051 peek(char **ps, char *es, char *toks)
9052 {
9053   char *s;
9054 
9055   s = *ps;
9056   while(s < es && strchr(whitespace, *s))
9057     s++;
9058   *ps = s;
9059   return *s && strchr(toks, *s);
9060 }
9061 
9062 struct cmd *parseline(char**, char*);
9063 struct cmd *parsepipe(char**, char*);
9064 struct cmd *parseexec(char**, char*);
9065 struct cmd *nulterminate(struct cmd*);
9066 
9067 struct cmd*
9068 parsecmd(char *s)
9069 {
9070   char *es;
9071   struct cmd *cmd;
9072 
9073   es = s + strlen(s);
9074   cmd = parseline(&s, es);
9075   peek(&s, es, "");
9076   if(s != es){
9077     printf(2, "leftovers: %s\n", s);
9078     panic("syntax");
9079   }
9080   nulterminate(cmd);
9081   return cmd;
9082 }
9083 
9084 struct cmd*
9085 parseline(char **ps, char *es)
9086 {
9087   struct cmd *cmd;
9088 
9089   cmd = parsepipe(ps, es);
9090   while(peek(ps, es, "&")){
9091     gettoken(ps, es, 0, 0);
9092     cmd = backcmd(cmd);
9093   }
9094   if(peek(ps, es, ";")){
9095     gettoken(ps, es, 0, 0);
9096     cmd = listcmd(cmd, parseline(ps, es));
9097   }
9098   return cmd;
9099 }
9100 struct cmd*
9101 parsepipe(char **ps, char *es)
9102 {
9103   struct cmd *cmd;
9104 
9105   cmd = parseexec(ps, es);
9106   if(peek(ps, es, "|")){
9107     gettoken(ps, es, 0, 0);
9108     cmd = pipecmd(cmd, parsepipe(ps, es));
9109   }
9110   return cmd;
9111 }
9112 
9113 struct cmd*
9114 parseredirs(struct cmd *cmd, char **ps, char *es)
9115 {
9116   int tok;
9117   char *q, *eq;
9118 
9119   while(peek(ps, es, "<>")){
9120     tok = gettoken(ps, es, 0, 0);
9121     if(gettoken(ps, es, &q, &eq) != 'a')
9122       panic("missing file for redirection");
9123     switch(tok){
9124     case '<':
9125       cmd = redircmd(cmd, q, eq, O_RDONLY, 0);
9126       break;
9127     case '>':
9128       cmd = redircmd(cmd, q, eq, O_WRONLY|O_CREATE, 1);
9129       break;
9130     case '+':  // >>
9131       cmd = redircmd(cmd, q, eq, O_WRONLY|O_CREATE, 1);
9132       break;
9133     }
9134   }
9135   return cmd;
9136 }
9137 
9138 
9139 
9140 
9141 
9142 
9143 
9144 
9145 
9146 
9147 
9148 
9149 
9150 struct cmd*
9151 parseblock(char **ps, char *es)
9152 {
9153   struct cmd *cmd;
9154 
9155   if(!peek(ps, es, "("))
9156     panic("parseblock");
9157   gettoken(ps, es, 0, 0);
9158   cmd = parseline(ps, es);
9159   if(!peek(ps, es, ")"))
9160     panic("syntax - missing )");
9161   gettoken(ps, es, 0, 0);
9162   cmd = parseredirs(cmd, ps, es);
9163   return cmd;
9164 }
9165 
9166 struct cmd*
9167 parseexec(char **ps, char *es)
9168 {
9169   char *q, *eq;
9170   int tok, argc;
9171   struct execcmd *cmd;
9172   struct cmd *ret;
9173 
9174   if(peek(ps, es, "("))
9175     return parseblock(ps, es);
9176 
9177   ret = execcmd();
9178   cmd = (struct execcmd*)ret;
9179 
9180   argc = 0;
9181   ret = parseredirs(ret, ps, es);
9182   while(!peek(ps, es, "|)&;")){
9183     if((tok=gettoken(ps, es, &q, &eq)) == 0)
9184       break;
9185     if(tok != 'a')
9186       panic("syntax");
9187     cmd->argv[argc] = q;
9188     cmd->eargv[argc] = eq;
9189     argc++;
9190     if(argc >= MAXARGS)
9191       panic("too many args");
9192     ret = parseredirs(ret, ps, es);
9193   }
9194   cmd->argv[argc] = 0;
9195   cmd->eargv[argc] = 0;
9196   return ret;
9197 }
9198 
9199 
9200 // NUL-terminate all the counted strings.
9201 struct cmd*
9202 nulterminate(struct cmd *cmd)
9203 {
9204   int i;
9205   struct backcmd *bcmd;
9206   struct execcmd *ecmd;
9207   struct listcmd *lcmd;
9208   struct pipecmd *pcmd;
9209   struct redircmd *rcmd;
9210 
9211   if(cmd == 0)
9212     return 0;
9213 
9214   switch(cmd->type){
9215   case EXEC:
9216     ecmd = (struct execcmd*)cmd;
9217     for(i=0; ecmd->argv[i]; i++)
9218       *ecmd->eargv[i] = 0;
9219     break;
9220 
9221   case REDIR:
9222     rcmd = (struct redircmd*)cmd;
9223     nulterminate(rcmd->cmd);
9224     *rcmd->efile = 0;
9225     break;
9226 
9227   case PIPE:
9228     pcmd = (struct pipecmd*)cmd;
9229     nulterminate(pcmd->left);
9230     nulterminate(pcmd->right);
9231     break;
9232 
9233   case LIST:
9234     lcmd = (struct listcmd*)cmd;
9235     nulterminate(lcmd->left);
9236     nulterminate(lcmd->right);
9237     break;
9238 
9239   case BACK:
9240     bcmd = (struct backcmd*)cmd;
9241     nulterminate(bcmd->cmd);
9242     break;
9243   }
9244   return cmd;
9245 }
9246 
9247 
9248 
9249 
