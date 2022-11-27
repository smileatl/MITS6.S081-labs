//包含一些类型定义
#include "kernel/types.h"
// user.h提供了许多用户级程序的声明
#include "user/user.h"

// argc标明数组的成员数量，argv字符串参数数组的每个成员都是char*类型
int main(int argc, char const *argv[])
{
    // 只需要两个输入参数
    // argv[0]="sleep"，（多数程序忽略参数数组中的第一个元素，它通常是程序名）
    // argv[1]="10"，
    // argv[2]=null，null指针表明数组的结束，不包括在数组的数量之内

    //如果输入参数的数量不为2，说明参数错误
    if (argc != 2)
    {
        fprintf(2, "usage: sleep <time>\n");
        exit(1);
    }
    sleep(atoi(argv[1]));
    exit(0);
}