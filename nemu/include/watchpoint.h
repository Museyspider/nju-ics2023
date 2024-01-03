#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#define NR_WP 32

typedef struct watchpoint
{
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
  // 监视点信息
  // 类型 : 断点 或 监视点
  // 监视点显示设置
  // 监视点是否启用
  // 监视点的内存地址
  // 监视点监视的内容
  // 监视点的表达式
  // 执行指令之前监视点的值
  uint32_t expr_addr; // 存放表达式结果 , 即内存地址
  uint32_t val;       // 执行指令之前的值

} WP;

WP *new_wp();         // 添加一个监视点
void free_wp(WP *wp); // 删除一个监视点
int watchpoint_val(); // 判断所有监视点的值是否发生变化
int del_watchpoint(int num);
void print_head_free_();

#endif