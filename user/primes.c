
//my solution
#include "kernel/types.h"
#include "user/user.h"


//filter prime in each pipe
void filter(int p_left[])
{
    close(p_left[1]);
    
    int p_right[2];
    pipe(p_right);
            
    //read the left and write the right
    int buf;
    //update the prime
    read(p_left[0], &buf, sizeof(int)); 
    int prime = buf;

    if(prime == 0){
        close(p_left[0]);
        close(p_right[1]);      
        exit(0);
    }
    printf("prime %d\n", prime);        //print the prime
    
    
    if(fork() == 0) //child process
    {
        close(p_right[1]);
        close(p_left[0]);
        // close(p_left[1]);
        // printf("pright:%d\n",read(p_right[0], &buf, sizeof(int))); 
        filter(p_right);
            
    }
    else{   //father process
        
        while(read(p_left[0], &buf, sizeof(int))){
            if(buf % prime !=0){
                // printf("%d\n",buf);
                write(p_right[1], &buf, sizeof(int));
            }
        }
        close(p_left[0]);
        
        close(p_right[1]);
        wait(0);
        exit(0);
    }

}

int main(void)
{
    
    int p[2];
    pipe(p);
    //write the right (initial input)

    printf("prime %d\n", 2);        //print the prime 
    for(int i = 2; i<=35; i++){
        if(i % 2 != 0){
            write(p[1], &i, sizeof(int));
        }
    }
    filter(p);
    exit(0);
}


//总结：要回收文件描述符，资源不够，read报错



// #include "kernel/types.h"
// #include "user/user.h"

// // 递归筛选素数的函数
// void filter(int p_left[]) {
//     int prime;
//     int buf;
    
//     // 读取第一个数作为 prime
//     if (read(p_left[0], &prime, sizeof(int)) == 0) {
//         close(p_left[0]);
//         exit(0);  // 没有数据，直接退出
//     }

//     // 打印当前 prime
//     printf("prime %d\n", prime);

//     // 创建新的管道
//     int p_right[2];
//     pipe(p_right);

//     // 创建子进程
//     if (fork() == 0) {  // 子进程
//         close(p_right[1]);  // 关闭写端
//         close(p_left[0]);    // 关闭左管道
//         filter(p_right);     // 递归处理下一个素数
//     } else {  // 父进程
//         close(p_right[0]);  // 关闭读端，父进程只写

//         // 读取管道数据，筛选后写入右管道
//         while (read(p_left[0], &buf, sizeof(int))) {
//             if (buf % prime != 0) {  // 过滤非 prime 的倍数
//                 write(p_right[1], &buf, sizeof(int));
//             }
//         }

//         close(p_left[0]);   // 关闭左管道
//         close(p_right[1]);  // 关闭右写端
//         wait(0);            // 等待子进程
//         exit(0);            // 退出
//     }
// }

// int main(void) {
//     int p[2];
//     pipe(p);

//     // 初始化数据流
//     if (fork() == 0) {  // 子进程
//         close(p[1]);  // 关闭写端
//         filter(p);    // 处理素数
//     } else {  // 父进程
//         close(p[0]);  // 关闭读端
//         for (int i = 2; i <= 35; i++) {
//             write(p[1], &i, sizeof(int));
//         }
//         close(p[1]);  // 关闭管道写端，通知子进程 EOF
//         wait(0);      // 等待子进程
//     }

//     exit(0);
// }
