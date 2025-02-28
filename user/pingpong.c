//管道的读写是单向的，
//如果要实现两个进程互相通信，需要建立两个单向管道

//my solution
#include "kernel/types.h"
#include "user/user.h"

int main(int argc, int *argv[])
{
    int PtoC[2];
    int CtoP[2];
    pipe(PtoC);
    pipe(CtoP);
    if(fork() != 0){
        int read_buf[1];
        close(CtoP[1]);
        write(PtoC[1], "A", 1);
        close(PtoC[1]);
        close(PtoC[0]);
        read(CtoP[0], read_buf, 1);
        printf("%d: received ping\n",getpid());
        close(CtoP[0]);

        wait(0);
        exit(0);
    }
    else{
        int read_buf[1];
        close(PtoC[1]);
        read(PtoC[0], read_buf, 1);
        printf("%d: received pong\n",getpid());
        write(CtoP[1], "B", 1);
        close(CtoP[1]);
        close(CtoP[0]);

        exit(0);
    }
}

