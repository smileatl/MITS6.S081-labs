#include "kernel/types.h"

// 磁盘上的文件系统格式。
// 内核和用户程序都使用这个头文件。
#include "kernel/fs.h"
// 有一个stat类型的结构体
#include "kernel/stat.h"
#include "user/user.h"

void find(char* path, const char* filename)
{
  char buf[512], * p;
  int fd;
  /*
  dirent结构体，目录是包含一连串dirent结构体的文件
  DIRSIZ定义为了14
    struct dirent {
    ushort inum;
    char name[DIRSIZ];
  };
  */
  struct dirent de;
  /*
  // 一个底层文件（叫做inode，索引结点）
  // Inode保存有关文件的元数据（用于解释或帮助理解信息的数据），
  // 包括其类型(文件/目录/设备)、长度、文件内容在磁盘上的位置以及指向文件的链接数
    struct stat {
    int dev;     // File system's disk device
    uint ino;    // Inode number
    short type;  // Type of file
    short nlink; // Number of links to file
    uint64 size; // Size of file in bytes
  };
  */
  struct stat st;

  //打开一个文件，0表示read，1表示write，返回一个文件描述符
  if ((fd = open(path, 0)) < 0) {
    //错误信息输出到文件描述符2（标准错误）
    fprintf(2, "find: cannot open %s\n", path);
    //return函数直接结束
    return;
  }

  //fstat系统调用从文件描述符所引用的inode中检索信息
  //将打开文件fd的信息放入st中
  if (fstat(fd, &st) < 0) {
    //返回值小于0表示出错
    fprintf(2, "find: cannot fstat %s\n", path);
    //及时关闭进程不需要的文件描述符
    close(fd);
    //return函数直接结束
    return;
  }

  //结构体stat类型的数据就是inode，inode中保存有关文件的元数据
  //参数错误，find的第一个参数必须是目录
  //st.type表示文件类型，T_DIR=1，表示目录
  if (st.type != T_DIR) {
    fprintf(2, "usage: find <DIRECTORY> <filename>\n");
    return;
  }

  //目录是包含一连串dirent结构体的文件,(DIRSIZ=14)+strlen(path)，
  //路径长度过长报错
  if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) {
    fprintf(2, "find: path too long\n");
    return;
  }
  //将path所指向的字符串复制到buf所指向的空间中，'\0'也会拷贝过去
  strcpy(buf, path);
  //buf是首地址，现在p指向buf数组最后一个数的后一个位置
  p = buf + strlen(buf);
  //*p='/'，buf数组最后一个数的后一个位置为'/'；p++，p指向最后一个'/'之后
  *p++ = '/'; //p指向最后一个'/'之后

  //将路径读入目录结构体，一级目录一级目录地读
  //有的读就一直读
  while (read(fd, &de, sizeof de) == sizeof de) {
    if (de.inum == 0)
      continue;
    //拷贝de.name所指向的内存内容的前14个字节到p所指的内存地址上
    //刚才的p指向了路径最后一个'/'之后，p还在buf数组当中
    memmove(p, de.name, DIRSIZ); //添加路径名称
    //p还指向buf数组中，p后面的第14个数置0为数组结束的标志，也就是字符串结束的标志
    //(其不包括在数组的大小之内)
    p[DIRSIZ] = 0;               //字符串结束标志
    //将指定名称的文件信息放入st中
    //buf是path/path/.../path...一直往下，直到找到file
    if (stat(buf, &st) < 0) {
      fprintf(2, "find: cannot stat %s\n", buf);
      continue;
    }
    //不要在“.”和“..”目录中递归
    //strcmp比较两个字符串的大小（比的是ASCII码），相等返回0，不相等返回ASCII差值
    //如果是目录就一直往下找
    //直到st.type等于T_FILE=2/(T_DEVICE=3)为止
    if (st.type == T_DIR && strcmp(p, ".") != 0 && strcmp(p, "..") != 0) {
      //buf是path/path/.../path...一直往下，直到找到file
      find(buf, filename);
    }
    //如果filename字符串和现在p所指向的字符数组相等
    //s（如果字符串是d，那剩下13个数都是0）
    else if (strcmp(filename, p) == 0)
      printf("%s\n", buf);
  }

  close(fd);
}

//argv字符串参数数组的每个成员都是char*类型
int main(int argc, char* argv[])
{
  // 只需要三个输入参数
  // argv[0]="find"，（多数程序忽略参数数组中的第一个元素，它通常是程序名）
  // argv[1]="directory"，
  // argv[2]="filename"

  //如果输入参数的数量不为3，说明用法错误，提示一下usage
  if (argc != 3) {
    fprintf(2, "usage: find <directory> <filename>\n");
    // 1表示失败
    exit(1);
  }
  //directory作为find的第一个输入参数，filename作为find的第二个输入采纳数
  find(argv[1], argv[2]);
  exit(0);
}
