#include "kernel/types.h"
#include "user/user.h"

#define RD 0 // pipe的read端
#define WR 1 // pipe的write端

int main(int argc, char const *argv[])
{
    char buf = 'p'; //用于传送的一个字节

    int fd_c2p[2]; //子进程->父进程，0是该管道的read文件描述符，1是该管道的write描述符
    int fd_p2c[2]; //父进程->子进程

    pipe(fd_c2p); //创建两个管道
    pipe(fd_p2c);

    // fork()系统调用创建一个新进程
    // 在父进程中，fork返回子类的PID，该pid>0
    // 在子进程中，fork返回零，该pid=0
    int pid = fork();
    int exit_status = 0;
    if (pid < 0)
    {   
        //小于0说明是错误的pid，打印错误信息
        fprintf(2, "fork() error!\n");
        //关闭管道
        close(fd_c2p[RD]);
        close(fd_c2p[WR]);
        close(fd_p2c[RD]);
        close(fd_p2c[WR]);
        //1表示失败
        exit(1);
    }
    else if (pid == 0)
    {
        //在子进程中：
        //p2c只需要read，c2p只需要write
        //对于不需要的管道描述符，要尽可能早地关闭
        //因为如果管道的写端没有close，管道中数据为空时对管道地读取将会阻塞
        close(fd_p2c[WR]);
        close(fd_c2p[RD]);

        //如果子进程先在p2c管道读取端read数据，如果失败
        if (read(fd_p2c[RD], &buf, sizeof(char)) != sizeof(char))
        {
            //fprintf：第二个参数字符串来转换并格式化数据，并把结果输出到第一个参数指定的文件中
            //错误信息输出到文件描述符2（标准错误）
            fprintf(2, "child read() error!\n");
            //退出状态1，表示错误
            exit_status = 1;
        }
        else
        {
            //成功信息输出到文件描述符1（标准输出）
            //getpid()获取当前进程的pid，该子进程pid为4
            fprintf(1, "%d: received ping\n", getpid());
        }

        //然后子进程在c2p管道写入端执行write操作
        if (write(fd_c2p[WR], &buf, sizeof(char)) != sizeof(char))
        {
            //失败打印错误信息
            fprintf(2, "child write() error!\n");
            //错误状态置为1，表示错误
            exit_status = 1;
        }

        //子进程中用完的管道描述符，也要记得关闭
        close(fd_p2c[RD]);
        close(fd_c2p[WR]);
        //如果退出状态为0表示成功，为1表示失败
        exit(exit_status);
    }
    else
    {
        //在父进程中：
        //p2c只需要write，c2p只需要read
        //对于不需要的管道描述符，要尽可能早地关闭
        close(fd_p2c[RD]);
        close(fd_c2p[WR]);

        //父进程先在p2c管道的写入端执行write操作
        if (write(fd_p2c[WR], &buf, sizeof(char)) != sizeof(char))
        {
            fprintf(2, "parent write() error!\n");
            exit_status = 1;
        }

        //然后在c2p管道的读取端执行read操作
        if (read(fd_c2p[RD], &buf, sizeof(char)) != sizeof(char))
        {
            fprintf(2, "parent read() error!\n");
            exit_status = 1;
        }
        else
        {
            //该父进程pid为3
            fprintf(1, "%d: received pong\n", getpid());
        }

        close(fd_p2c[WR]);
        close(fd_c2p[RD]);

        exit(exit_status);
    }
}