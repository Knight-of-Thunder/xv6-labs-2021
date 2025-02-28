
//my solution
#include "kernel/types.h"
#include "user/user.h"
#include "kernel/param.h"

int main(int argc, char *argv[]){
  
  int rd = 1;
  while(1){
    //read from the sd input to get args
    char *xargv[MAXARG];
    for(int i = 1; i < argc; i++){
      xargv[i-1] = argv[i];
    }
    char buf[512];
    char *t = buf;
    
    do{
      rd = read(0, t, 1);
      if(rd < 0){
        fprintf(2,"read wrong\n");
        exit(1);
      }

    }while(*t++ != '\n' && rd != 0); 
    
    if(rd == 0){
        break;
      }
    *--t = 0;
    xargv[argc - 1] = buf;
    xargv[argc] = 0;

    //exec in child process
    if(fork() == 0){
      exec(xargv[0], xargv);
      exit(0);
    }
    else{
      wait(0);
    }
  }
  exit(0);
}


//echo的输出每一行的末尾都是 \n ,包括最后一行
//读取参数时 舍弃 \n
