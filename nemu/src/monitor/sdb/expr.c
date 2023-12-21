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

#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

// ---------
#include <stdbool.h>

extern int strtoval(char *);
// --------- 

enum {
  TK_NOTYPE = 256, TK_EQ, TK_NUM, TK_PLUS, TK_MINUS, TK_MUL, TK_DIV, TK_L, TK_R 

  /* TODO: Add more token types */

};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", TK_PLUS},         // plus
  {"==", TK_EQ},        // equal

  // -----------
  {"\\-", TK_MINUS},
  {"\\*", TK_MUL},
  {"\\/", TK_DIV},
  {"[0-9]+",  TK_NUM},
  {"\\(", TK_L},
  {"\\)", TK_R},
  // -----------
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

// 识别token
static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {

      // 这里它是确定了 一个要匹配的子串
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {

          // -------------
          case TK_NOTYPE: 
            break;

          case TK_NUM: 
            for(int i = 0; i < substr_len; i ++)
            {
              tokens[nr_token].str[i] = *(substr_start + i);
            }
            tokens[nr_token].type = rules[i].token_type;
            nr_token++;
            break;

          case TK_PLUS: 
            tokens[nr_token].str[0] = *(substr_start);
            tokens[nr_token].type = rules[i].token_type;
            nr_token++;
            break;

          case TK_MINUS: 
            tokens[nr_token].str[0] = *(substr_start);
            tokens[nr_token].type = rules[i].token_type;
            nr_token++;
            break;

          case TK_MUL: 
            tokens[nr_token].str[0] = *(substr_start);
            tokens[nr_token].type = rules[i].token_type;
            nr_token++;
            break;
          
          case TK_DIV: 
            tokens[nr_token].str[0] = *(substr_start);
            tokens[nr_token].type = rules[i].token_type;
            nr_token++;
            break;

          case TK_R: 
            tokens[nr_token].str[0] = *(substr_start);
            tokens[nr_token].type = rules[i].token_type;
            nr_token++;
            break;

          case TK_L: 
            tokens[nr_token].str[0] = *(substr_start);
            tokens[nr_token].type = rules[i].token_type;
            nr_token++;
            break;

          // -------------
          default: break;
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

static bool check_parentheses(int p, int q)
{
  if(tokens[p].type != TK_L)
  {
    return false;
  }
  int L = 1;
  // int error = 0;
  int count = 0;  // L 为0的次数只能是1次
  for(int i = p + 1; i <= q; i ++)
  {
    // printf("L=%d\n", L);
    if(tokens[i].type==TK_L)
    {
      L ++;
    }
    if(tokens[i].type==TK_R)
    {
      L --;
    }
    if(L < 0)
    {
      // error = 1;
      printf("表达式错误！");
      return false;
    }
    if(L == 0)
    {
      count++;
    }
  }
  if(L == 0 && count == 1)
  {
    return true;
  }
  // printf("count=%d\n", count);
  return false;
}

// 没有进行异常表达式的处理
static int optPosition(int p, int q)
{
  int level = 0;
  int pos = p;
  int L = 0;
  for(int i = p; i <= q; i ++)
  {
    if(tokens[i].type == TK_L)
    {
      L ++;
    }
    if(tokens[i].type == TK_R)
    {
      L --;
    }
    if(L == 0 && (tokens[i].type == TK_DIV || tokens[i].type == TK_MUL) && level == 0)
    {
      pos = i;
    }
    if(L == 0 && (tokens[i].type == TK_PLUS || tokens[i].type == TK_MINUS))
    {
      level ++;
      pos = i;
    }
  }
  return pos;
}


word_t eval(int p, int q)
{
  if (p > q) 
  {
    // Bad expression 
    return 0;  // 是不是返回0
  }
  else if (p == q) 
  {
    /* Single token.
     * For now this token should be a number.
     * Return the value of the number.
     */
    return strtoval(tokens[p].str);
  }
  else if (check_parentheses(p, q) == true) {
    /* The expression is surrounded by a matched pair of parentheses.
     * If that is the case, just throw away the parentheses.
     */
    return eval(p + 1, q - 1);
  }
  else {
    int op = optPosition(p, q);
    int val1 = eval(p, op - 1);
    int val2 = eval(op + 1, q);

    switch (tokens[op].type) {
      case TK_PLUS: return val1 + val2;
      case TK_MINUS: return val1 - val2;
      case TK_MUL: return val1 * val2;
      case TK_DIV: return val1 / val2;
      default: assert(0);
    }
  }
}


word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }
  // for(int i = 0; i < nr_token; i ++)
  // {
  //   printf("%s\n", tokens[i].str);
  // }

  // printf("%d\n", check_parentheses(0, nr_token - 1));
  // printf("optPosition=%d\n", optPosition(0, nr_token - 1));

  /* TODO: Insert codes to evaluate the expression. */
  // TODO();
  printf("expr=%d\n", eval(0, nr_token - 1));

  return 0;
}
