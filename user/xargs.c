#include "kernel/types.h"
#include "user/user.h"
#include "kernel/param.h"

#define MAXSZ 512
// 有限状态自动机状态定义
enum state {
  S_WAIT,         // 等待参数输入，此状态为初始状态或当前字符为空格
  S_ARG,          // 参数内
  S_ARG_END,      // 参数结束
  S_ARG_LINE_END, // 左侧有参数的换行，例如"arg\n"
  S_LINE_END,     // 左侧为空格的换行，例如"arg  \n""
  S_END           // 结束，EOF
};

// 字符类型定义
enum char_type {
  C_SPACE,        //空格
  C_CHAR,         //字符
  C_LINE_END      //换行
};

/**
 * @brief 获取字符类型
 *
 * @param c 待判定的字符
 * @return enum char_type 字符类型
 */
enum char_type get_char_type(char c)
{
  switch (c) {
  case ' ':
    return C_SPACE;      //空格
  case '\n':
    return C_LINE_END;   //换行
  default:
    return C_CHAR;       //字符
  }
}

/**
 * @brief 状态转换
 *
 * @param cur 当前的状态
 * @param ct 将要读取的字符
 * @return enum state 转换后的状态
 */
enum state transform_state(enum state cur, enum char_type ct)
{
  switch (cur) {
  case S_WAIT: //在等待参数输入的状态时
    //读取到空格，状态由S_WAIT变为S_WAIT，继续等待参数到来
    if (ct == C_SPACE)    return S_WAIT;
    //读取到换行，状态变为S_LINE_END：左侧为空格的换行
    if (ct == C_LINE_END) return S_LINE_END;
    //读取到字符，状态变为S_ARG：在参数内
    if (ct == C_CHAR)     return S_ARG;
    break;
  case S_ARG: //在参数内的状态时
    //读取到空格，说明参数结束，状态变为S_ARG_END
    if (ct == C_SPACE)    return S_ARG_END;
    //读取到换行，状态变为S_ARG_LINE_END：左侧有参数的换行
    if (ct == C_LINE_END) return S_ARG_LINE_END;
    //读取到字符，就还是在参数内的状态
    if (ct == C_CHAR)     return S_ARG;
    break;
  case S_ARG_END:
  case S_ARG_LINE_END:
  case S_LINE_END: //左侧为空格的换行状态时
    //读取到空格，变为等待参数输入状态
    if (ct == C_SPACE)    return S_WAIT;
    //读取到换行时，还是左侧为空格的换行状态
    if (ct == C_LINE_END) return S_LINE_END;
    //读取到字符，变为在参数内的状态
    if (ct == C_CHAR)     return S_ARG;
    break;
  default:
    break;
  }
  return S_END;
}


/**
 * @brief 将参数列表后面的元素全部置为空
 *        用于换行时，重新赋予参数
 *
 * @param x_argv 参数指针数组
 * @param beg 要清空的起始下标
 */
void clearArgv(char* x_argv[MAXARG], int beg)
{
  for (int i = beg; i < MAXARG; ++i)
    x_argv[i] = 0;
}


int main(int argc, char* argv[])
{
  //输入参数太多，打印错误信息到标准输出
  if (argc - 1 >= MAXARG) {
    fprintf(2, "xargs: too many arguments.\n");
    exit(1);
  }
  char lines[MAXSZ];
  char* p = lines;
  char* x_argv[MAXARG] = { 0 }; // 参数指针数组，全部初始化为空指针

  // 存储原有的参数
  for (int i = 1; i < argc; ++i) {
    //把输入参数放入新建的参数指针数组x_argv
    //argv的第0个参数一般是程序名，这里是"xargs"
    x_argv[i - 1] = argv[i];
  }
  int arg_beg = 0;          // 参数起始下标
  int arg_end = 0;          // 参数结束下标
  int arg_cnt = argc - 1;   // 当前参数索引
  enum state st = S_WAIT;   // 起始状态置为S_WAIT

  //只要状态不为结束，就循环执行
  while (st != S_END) {
    // 读取为空则退出
    // 从标准输入中读取，放入指针p指向的数组lines[MAXSZ]
    if (read(0, p, sizeof(char)) != sizeof(char)) {
      st = S_END;
    }
    else {
      // 读取正确就去执行状态转换
      st = transform_state(st, get_char_type(*p));
    }

    //参数太长的错误信息
    if (++arg_end >= MAXSZ) {
      fprintf(2, "xargs: arguments too long.\n");
      exit(1);
    }

    switch (st) {
    case S_WAIT:          // 这种情况下只需要让参数起始指针移动
      ++arg_beg;
      break;
    case S_ARG_END:       // 参数结束，将参数地址存入x_argv数组中
      //接在输入参数数组后面，储存参数地址（将lines[arg_beg]的地址存入x_argv中）
      x_argv[arg_cnt++] = &lines[arg_beg];
      //将参数开始下标移到参数结束下标的位置，刚才已经++arg_end了
      arg_beg = arg_end;
      *p = '\0';          // 替换为字符串结束符
      break;
    case S_ARG_LINE_END:  // 将参数地址存入x_argv数组中同时执行指令  
      x_argv[arg_cnt++] = &lines[arg_beg];
      // 不加break，因为后续处理同S_LINE_END
    case S_LINE_END:      // 行结束，则为当前行执行指令
      arg_beg = arg_end;
      *p = '\0';
      if (fork() == 0) {
        //argv[0]="xargs", argv[1]是xargs后面紧跟着的那个程序名
        //这里是exec("grep", x_argv), x_argv[0]="grep", x_argv[1] = "hello"
        exec(argv[1], x_argv);
      }
      //将参数列表后面的元素全部置为空，现在是换行，要重新赋予参数
      arg_cnt = argc - 1;
      clearArgv(x_argv, arg_cnt);
      //等待子进程的退出
      wait(0);
      break;
    default:
      break;
    }

    ++p;    // 下一个字符的存储位置后移
  }
  exit(0);
}
