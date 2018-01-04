
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int exec_at(char *tty_dev, char *at)
{
   char cmdline[100] ={0} ;
   sprintf(cmdline, "/system/bin/busybox echo -e \"%s\r\n\" > %s", at, tty_dev);
   return  system(cmdline);
}


