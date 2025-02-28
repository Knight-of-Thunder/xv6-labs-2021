// #include "kernel/types.h"
// #include "kernel/stat.h"
// #include "user/user.h"
// #include "kernel/fs.h"

// char*
// fmtname(char *path)  //将路径转换为文件名，即将中间的/去掉
// {
// 	static char buf[DIRSIZ+1];
// 	char *p;
	
// 	// Find first character after last slash.
// 	//从后往前，查找最后一个斜杠之后的第一个字符，也就是文件名的首字母
// 	for(p=path+strlen(path); p >= path && *p != '/'; p--)
// 		;
// 	p++;
	
// 	// Return blank-padded name.
// 	//返回空白填充的名称
// 	//如果文件名的长度大于DIRSIZ，则直接返回p。p的长度小于DIRSIZ，则在p的文件名后面补齐空格
// 	if(strlen(p) >= DIRSIZ)
// 		return p;
// 	memmove(buf, p, strlen(p));//从存储区p复制stelen(p)个字节到存储区buf
// 	buf[strlen(p)]=0;//这是与ls中fmtname不同的地方，因为需要比较字符串，不再填充字符串p
// 	return buf;
// }

// void find(char *path,char *name)
// {
//     char buf[512], *p;
//     int fd;
//     struct dirent de;
//     struct stat st;
//     if((fd=open(path,0))<0){
//         fprintf(2,"find open %s error\n",path);
//         exit(1);
//     }
//     if(fstat(fd,&st)<0){
//         fprintf(2,"find %s error\n",path);
//         close(fd);
//         exit(1);
//     }
//     switch (st.type)
//     {
//     case T_FILE:
//         if(strcmp(fmtname(path),name)==0)
//         {
//             printf("%s\n",path);//当前路径与文件名相同，找到该文件
//         }
//         break;
//     case T_DIR:
//         if(strlen(path)+1+DIRSIZ+1>sizeof buf){
//             printf("find:path too long\n");
//             break;
//         }
//         strcpy(buf,path);
//         p=buf+strlen(buf);
//         *p++='/';
//         while(read(fd,&de,sizeof(de))==sizeof(de)){
//             if(de.inum==0)
//                 continue;
//             memmove(p,de.name,DIRSIZ);
//             p[DIRSIZ]=0;
//             //
//             if(!strcmp(de.name,".")||!strcmp(de.name,".."))
//                 continue;
//             find(buf,name);//继续进入下一层目录递归处理
//         }
//         break;
//     }
//     close (fd);
// }
// int
// main(int argc, char *argv[])
// {
//   if(argc != 3){
//     fprintf(2, "usage:find <path> <name>\n");
//     exit(1);
//   }
//     find(argv[1], argv[2]);
//     exit(0);
// }


//my solution
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char*
fmtname(char *path)
{
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  buf[strlen(p)]=0;
  return buf;
}

void
find(char *path, char *name)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if((fd = open(path, 0)) < 0){
    fprintf(2, "ls: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    fprintf(2, "ls: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch(st.type){
  case T_FILE:
    if(strcmp(fmtname(path),name)==0)
    {
        printf("%s\n",path);//当前路径与文件名相同，找到该文件
    }
    break;

  case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf("ls: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){
        printf("find: cannot stat %s\n", buf);
        continue;
      }
        if(!strcmp(de.name,".")||!strcmp(de.name,".."))
            continue;
        find(buf, name);
    }
    break;
  }
  close(fd);
}

int
main(int argc, char *argv[])
{
  if(argc != 3){
    fprintf(2, "usage:find <path> <name>\n");
    exit(1);
  }
    find(argv[1], argv[2]);
    exit(0);
}


//与ls的区别：
//find的fmt函数要把名字转化成字符串，末尾直接加0.
//避免递归搜索. ..