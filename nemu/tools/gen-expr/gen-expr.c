/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>
// ---------
#include <sys/time.h>

// this should be enough
static char buf[65536] = {};
static int bufIndex = 0;
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";

static int choose(int maxvalue)
{
    int randomNumber;
    // 设置随机数种子
    struct timeval tv;
    gettimeofday(&tv, NULL);

    unsigned int seed = tv.tv_sec * 1000000 + tv.tv_usec;
    srand(seed);
    // 生成随机数
    randomNumber = rand() % maxvalue;  // 得到 0 到 2 之间的随机整数
    // printf("Random number: %d\n", randomNumber);
    return randomNumber;
}


static void gen_num()
{
  int randomNumber;
  // 设置随机数种子
  struct timeval tv;
  gettimeofday(&tv, NULL);

  unsigned int seed = tv.tv_sec * 1000000 + tv.tv_usec;
  srand(seed);
  // 生成随机数
  randomNumber = rand() % 1000;  // 得到 0 到 2 之间的随机整数
  int b = randomNumber / 100;
  int s = randomNumber % 100 / 10;
  int g = randomNumber % 10;
  if(b == 0 && s == 0 && g == 0)
  {
    buf[bufIndex] = '0';
    bufIndex ++ ;
  }
  if(b == 0 && s == 0)
  {
    buf[bufIndex] = '0' + g;
    bufIndex ++ ;
  }
  if(b == 0)
  {
    buf[bufIndex] = '0' + s;
    buf[bufIndex + 1] = '0' +  g;
    bufIndex += 2 ;
  }
  buf[bufIndex] = '0' + b;
  buf[bufIndex + 1] = '0' + s;
  buf[bufIndex + 2] = '0' + g;
  bufIndex += 3;
}

static void gen_rand_op()
{
    int randomNumber;
    // 设置随机数种子
    struct timeval tv;
    gettimeofday(&tv, NULL);

    unsigned int seed = tv.tv_sec * 1000000 + tv.tv_usec;
    srand(seed);
    // 生成随机数
    randomNumber = rand() % 4 + 259;  // 得到 0 到 3 之间的随机整数
    // printf("randomNumber=%d\n", randomNumber);
    // printf("Random number: %d\n", randomNumber);
    switch (randomNumber)
    {
      case 259 : buf[bufIndex] = '+'; bufIndex ++; break;   //  TK_PLUS
      case 260 : buf[bufIndex] = '-'; bufIndex ++; break;
      case 261 : buf[bufIndex] = '*'; bufIndex ++; break;
      case 262 : buf[bufIndex] = '/'; bufIndex ++; break;
    }
    // assert(0);
}

static void gen(char s)
{
  if(s == '(')
  {
    buf[bufIndex] = '(';
  }
  if(s == ')')
  {
    buf[bufIndex] = ')';
  }
  bufIndex ++;
}

static void gen_rand_expr() 
{
  switch (choose(3)) {
    case 0: gen_num(); break;
    case 1: gen('('); gen_rand_expr(); gen(')'); break;
    default: gen_rand_expr(); gen_rand_op(); gen_rand_expr(); break;
  }
}

int main(int argc, char *argv[]) {
  struct timeval tv;
  gettimeofday(&tv, NULL);

  unsigned int seed = tv.tv_sec * 1000000 + tv.tv_usec;
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {

    gen_rand_expr();
    buf[bufIndex] = '\0';
    // printf("%s\n", buf);

    sprintf(code_buf, code_format, buf);
    // printf("%s\n", code_buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc /tmp/.code.c -o /tmp/.expr");
    if (ret != 0) continue;

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    int result;
    ret = fscanf(fp, "%d", &result);
    pclose(fp);

    printf("%u %s\n", result, buf);

    // buf[0] = '\0';
    bufIndex = 0;
  }
  return 0;
}
