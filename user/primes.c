#include "kernel/types.h"
#include "user/user.h"

#define RD 0
#define WR 1

//INT_LEN为4
const uint INT_LEN = sizeof(int);

/**
 * @brief 读取左邻居的第一个数据
 * @param lpipe 左邻居的管道符
 * @param pfirst 用于存储第一个数据的地址
 * @return 如果没有数据返回-1,有数据返回0
 */
int lpipe_first_data(int lpipe[2], int* dst)
{
  //从左邻居中读取数据，一旦读取到就打印并结束（也就是读取了第一个数据）
  if (read(lpipe[RD], dst, sizeof(int)) == sizeof(int)) {
    printf("prime %d\n", *dst);
    //return 0表示成功
    return 0;
  }
  return -1;
}

/**
 * @brief 读取左邻居的数据，将不能被first整除的写入右邻居
 * @param lpipe 左邻居的管道符
 * @param rpipe 右邻居的管道符
 * @param first 左邻居的第一个数据
 */
void transmit_data(int lpipe[2], int rpipe[2], int first)
{
  int data;
  // 从左管道读取数据，读入到data中，只要有数据就一直读，一共读取了34次（2...35）
  while (read(lpipe[RD], &data, sizeof(int)) == sizeof(int)) {
    // 将无法整除的数据传递入右管道
    // 第一次运行该函数是在第二个进程中
    if (data % first)
      write(rpipe[WR], &data, sizeof(int));
  }
  //关闭左管道的read端，因为左管道write端在primes中已经关闭
  close(lpipe[RD]);
  //关闭右管道的write端，写完就关，省的xv6系统资源不足
  close(rpipe[WR]);
}

/**
 * @brief 寻找素数
 * @param lpipe 左邻居管道
 */
void primes(int lpipe[2])
{
  //将传入的管道作为左邻居
  //这时候的管道需要read，不需要write，相当于文件描述符4被关了，此时最小的未被使用的描述符就是4
  close(lpipe[WR]);
  int first;
  //执行lpipe_first_data(lpipe, &first)后，first就获取到了左邻居中的第一个数据
  //判断一下返回值是否为0，0才说明成功
  //一直读到无数可读，返回-1的时候
  if (lpipe_first_data(lpipe, &first) == 0) {
    int p[2];
    //在第二个进程新建一个管道（其pid=4）
    pipe(p); // 当前的管道
    //从左邻居中读取数据，将不能被first整除的写入右邻居
    transmit_data(lpipe, p, first);

    if (fork() == 0) {
      //子进程的子进程，孙子（现在的第三个进程（其pid为5））
      //printf("孙：%d\n",getpid());
      primes(p);    // 递归的思想，但这将在一个新的进程中调用
    }
    else {
      //pid=4的进程，也就是第二个进程创建的管道的read端还没有关闭
      close(p[RD]);
      //等待子进程的退出，但是子进程还要等待其子进程的退出，就相当于等待所有子孙进程的退出
      wait(0);
    }
  }
  exit(0);
}

int main(int argc, char const* argv[])
{
  //初始化时，p[0]={0,0}
  int p[2];
  //创建一个管道，p[]={3,4}，当前进程中编号最小的未使用的描述符是3、4
  //所以该管道的read端的文件描述符是3，write端的文件描述符是4
  pipe(p);

  // printf("父：%d\n",getpid());
  //i是整型，4个字节；一个write相当于从i写4个字节到p管道的write端
  //这第一个进程（其pid为3）将数字2到35输入到管道中（直接将32位（4字节）int写入管道，而不是使用格式化的ASCII I/O）
  for (int i = 2; i <= 35; ++i) //写入初始数据
    write(p[WR], &i, INT_LEN);

  if (fork() == 0) {
    //子进程中（现在的第二个进程（其pid为4））
    // printf("子：%d\n", getpid());
    primes(p);
  }
  else {
    //父进程，pid=3的进程，要关闭管道的write和read端
    close(p[WR]);
    close(p[RD]);
    wait(0);
  }
  exit(0);
}
