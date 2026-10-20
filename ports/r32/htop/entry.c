#include <stdlib.h>
int CommandLine_run(int,char **);
int app_main(int n,char **v){setenv("HTOPRC","/root/htop.rc",0);return CommandLine_run(n,v);}
