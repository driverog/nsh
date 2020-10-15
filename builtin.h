//
// Created by daniel on 2020-03-17.
//

#ifndef NSH_BUILTIN_H
#define NSH_BUILTIN_H

#include <stddef.h>
#include <unistd.h>
#include <stdio.h>
#include "list.h"
#include "glist.h"
#include <string.h>
#include <fcntl.h>
#include <wait.h>

int current_pid_wait;
int to_kill_grp;
List* bg_pid_list;
List* to_wait_pid;
List* bg_grp_list;
GList* dict_keys;
GList* dict_content;
GList* bg_pname_list;
char* nsh_history_path;
GList* history_list;
int last_ctr_c_pid;
int waiting_bg;
int last_waiting_bg;
int ctrc_no_moreline;

int nsh_cd(char** args);
int nsh_exit(char** args);
int nsh_jobs(char** args);
int nsh_print_history(char** args);
int nsh_foreground(char** args);
int nsh_true(char** args);
int nsh_false(char** args);
int nsh_if(char** args);
int nsh_help(char** args);
int nsh_set(char** args);
int nsh_get(char** args);
int nsh_unset(char** args);
int nsh_if_keyword(char* word);
int nsh_if_keyword_2(char* word);
void nsh_print_args(char** args);

int nsh_launch(char** args, int fd_in, int fd_out, int to_close_1, int to_close_2);
int nsh_pipe_launch(char** args);
int nsh_is_special_char(char c);
int nsh_execute(char** args, int should_wait);


int bg_group_id;


char* builtin_str[];

int (*builtin_func[]) (char**);

char* builtin_str_out[];

int (*builtin_func_out[]) (char**);

char* nsh_get_home_adr();

void nsh_set_history_path();

int nsh_num_builtin();

int nsh_num_builtin_out();


#endif //NSH_BUILTIN_H
