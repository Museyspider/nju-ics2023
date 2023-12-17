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
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"

//
#include <memory/paddr.h>
#include <string.h>
#include <math.h>
//

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  // -----  如果程序没结束 让其执行一条ebreak指令
  if(nemu_state.state == NEMU_END || nemu_state.state == NEMU_ABORT)
  {
    return -1;
  }
  static const uint32_t ebreak = 0x00100073;
  memcpy(guest_to_host(RESET_VECTOR), &ebreak, 4);
  cpu_exec(1);
  // -----
  return -1;
}

static int cmd_si(char *args) {
  if(args == NULL)
  {
    cpu_exec(1);
    return 0;
  }
  int len = strlen(args);
  int num = 0;
  for(int i = 0; i < len; i ++)
  {
    if(args[i] - '0' > 9 || args[i] - '0' < 0)
    {
      // cuo wu zi fu
      // printf("-----------");
      return 1;
    }
    num += (args[i] - '0') * pow(10,len - i - 1);  
  }
  cpu_exec(num);

  return 0;
}

static int cmd_info(char *args) {
  int len = strlen(args);
  if(len > 1)
  {
    // 参数错误
    return 1;
  }
  if(args[0] == 'r')
  {
    isa_reg_display();
  }
  if(args[0] == 'w')
  {

  }
  return 0;
}

static int cmd_x(char *args) {
  cpu_exec(-1);
  return 0;
}

static int cmd_p(char *args) {
  cpu_exec(-1);
  return 0;
}

static int cmd_w(char *args) {
  cpu_exec(-1);
  return 0;
}

static int cmd_d(char *args) {
  cpu_exec(-1);
  return 0;
}

static int cmd_help(char *args);

// 程序中存在哪些命令
static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },

  /* TODO: Add more commands */
  { "si", "Exit NEMU", cmd_si },
  { "info", "Exit NEMU", cmd_info },
  { "x", "Exit NEMU", cmd_x },
  { "p", "Exit NEMU", cmd_p },
  { "w", "Exit NEMU", cmd_w },
  { "d", "Exit NEMU", cmd_d },
};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     * + 1 是去掉空格
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }  // 命令q是直接return  函数值小于0直接结束  
        break;  // 一条命令执行完
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
