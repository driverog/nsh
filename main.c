#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <wait.h>
#include <fcntl.h>

#include "builtin.h"
#include "list.h"
#include "glist.h"

#define BOLD_CYAN "\033[1;36m"
#define WHITE "\033[0m"
#define BOLD_RED "\033[1;31m"

void nsh_loop();
char* nsh_read_line();
char* nsh_prepare_line(char* line);
char** nsh_split_line(char* line);

int should_wait_pipe;
int save_to_history;
int f_process;
int group_id_count;
char* current_process_name;
int null_entry;

int nsh_is_special_char(char c){
	return c == ' ' || c == '\n' || c == '>' || c == '<' || c == '|' || c == '&' || c == ';';
};

char* nsh_prepare_line(char* line){
	int i;
	int j = 0;
	int home_adr = 0;
	for (int k = 0; line[k] != 0; ++k) {
		if (line[k] == '~')
			home_adr++;
	}
	char* pline = (char*)malloc(strlen(line)* 3 * sizeof(char) + home_adr*strlen(nsh_get_home_adr())* sizeof(char)  + 10*
	                                                                                                                  sizeof(char));
	int str_cont = 0;
	int s_str_cont = 0;
	
	for (i = 0; i < strlen(line); ++i) {
		if (line[i] == '\"'){
			if (str_cont % 2 == 1){
				if (s_str_cont % 2 == 0 && nsh_is_special_char(line[i+1])){
					pline[j++] = line[i];
					str_cont++;
					continue;
				}
				if (line[i+1] == '\"'){
					s_str_cont++;
					continue;
				}
				char c = line[i];
				line[i] = line[i + 1];
				line[i+1] = c;
				i--;
				continue;
			}
			else str_cont++;
		}
		if (str_cont % 2 == 0){
			if (line[i] == '#')
				break;
			if (line[i] == '~' && (i == 0 || line[i-1] == ' ') && (nsh_is_special_char(line[i+1]) || line[i+1] == '/'))
			{
				char* home_path = nsh_get_home_adr();
				pline[j] = 0;
				strcat(pline,home_path);
				j += (int)strlen(home_path);
				free(home_path);
				continue;
			}
			if (line[i] != '>' && line[i] != '<' && line[i] != '|' && line[i] != '&' && line[i] != ';')
				if ((i == 0 || pline[j-1] == ' ') && !nsh_is_special_char(line[i]))
				{
					pline[j++] = '\"';
					pline[j++] = line[i];
					line[i--] = '\"';
					str_cont++;
				}
				else{
					pline[j++] = line[i];
				}
			else{
				if (line[i] == '>' && line[i+1]  == '>')
				{
					pline[j++] = ' ';
					pline[j++] = line[i];
					pline[j++] = line[i++];
					pline[j++] = ' ';
				}
				else if (line[i] == '&' || line[i] == '|'){
					if (line[i+1] == line[i])
					{
						pline[j++] = ' ';
						pline[j++] = line[i++];
						pline[j++] = line[i];
						pline[j++] = ' ';
					}
					else {
						pline[j++] = ' ';
						pline[j++] = line[i];
						pline[j++] = ' ';
					}
				}
				else {
					pline[j++] = ' ';
					pline[j++] = line[i];
					pline[j++] = ' ';
				}
			}
		}
		else {
			pline[j++] = line[i];
		}
	}
	
	pline[j++] = 0;
	return pline;
}

void redirectOut(char* fileName){
	int fd = open(fileName,O_WRONLY | O_TRUNC | O_CREAT,0600);
	dup2(fd,STDOUT_FILENO);
	close(fd);
}

void redirectOutAppend(char* fileName){
	int fd = open(fileName,O_WRONLY | O_APPEND | O_CREAT,0600);
	dup2(fd,STDOUT_FILENO);
	close(fd);
}

void redirectIn(char* fileName){
	int fd = open(fileName,O_RDONLY);
	dup2(fd,STDIN_FILENO);
	close(fd);
}

int nsh_pipe_launch(char** args){
	int j = 0;
	int last_task = 0; // 0: nada o |, 1: &&, 2: ||
	int prev_status;
	int stacked_status = 0;
	int stop_exec = 0;
	int ret_val;
	
	to_wait_pid = createList();
	
	int fd_old[2];
	int fd_new[2];

	for (int i = 0; args[i] != NULL; ++i) {
		if (ctrc_no_moreline == 1)
			return 1;
		if ((i == j || (i - j > 1 && nsh_if_keyword_2(args[i-2]))) && strcmp(args[i],"if") == 0){
//			j = i;
			int c_if = 0;
			for (int k = i+1; args[k] != NULL ; ++k) {
				if (strcmp(args[k],"if") == 0 && (nsh_if_keyword(args[k-1]) || (k > 1 && nsh_if_keyword_2(args[k-2]))))
					c_if++;
				if (strcmp(args[k],"end") == 0){
					if (c_if == 0) {
						i = k;
						break;
					}
					c_if--;
				}
			}
			continue;
		}
		if (strcmp(args[i], "|") == 0 || strcmp(args[i], ";") == 0 || strcmp(args[i], "&&") == 0 || strcmp(args[i], "||") == 0) {
			char* info = malloc(3);
			strcpy(info,args[i]);
			args[i] = NULL;
			
			if (strcmp(info, "|") == 0) {
				pipe(fd_new);
				should_wait_pipe = 0;
			} else if (strcmp(info, ";") == 0) {
				fd_new[0] = -1;
				fd_new[1] = -1;
				should_wait_pipe = 0;
			} else if (strcmp(info, "&&") == 0 || strcmp(info, "||") == 0){
				fd_new[0] = -1;
				fd_new[1] = -1;
				should_wait_pipe = 1;
			}
			
			
			if (j > 0) {
				prev_status = nsh_launch(args + j, fd_old[0], fd_new[1], fd_old[1], fd_new[0]);
			} else {
				prev_status = nsh_launch(args + j, -1, fd_new[1], fd_new[0], -1);
			}
			
			if (last_task == 0){
				stacked_status = prev_status;
			} else if (last_task == 2){
				stacked_status = stacked_status || prev_status;
			} else if (last_task == 1){
				stacked_status = stacked_status && prev_status;
			}
			
			last_task = strlen(info) == 1 ? 0 : info[0] == '|' ? 2 : 1;
			
			
			if ((strcmp(info, "&&") == 0 && stacked_status != 0) || (strcmp(info, "||") == 0 && stacked_status == 0)){
				int more = 0;
				for (int k = i + 1; args[k] != NULL ; ++k) {
					if ((strcmp(args[k], "&&") == 0 && info[0] == '|') || (strcmp(args[k], "||") == 0 && info[0] == '&'))
					{
						last_task = args[k][0] == '|' ? 2 : 1;
						more = 1;
						i = k;
						break;
					}
				}
				stacked_status = info[0] == '&' ? 1 : 0;
				if (more == 0){
					stop_exec = 1;
					ret_val = (strcmp(info, "&&") == 0) ? 1 : 0;
					break;
				}
			}
			
			if (should_wait_pipe == 1){
				free(to_wait_pid);
				to_wait_pid = createList();
			}
			
			fd_old[0] = fd_new[0];
			fd_old[1] = fd_new[1];
			
			j = i + 1;
		}
	}
	
	if (stop_exec)
		return ret_val;
	
	should_wait_pipe = 1;
	
	if (j == 0){
		return nsh_launch(args,-1,-1,-1,-1);
	}
	else {
		return nsh_launch(args + j,fd_new[0],-1,fd_new[1],-1);
	}
}

void controlador(int a) {

}

int nsh_launch(char** args,int fd_in, int fd_out, int to_close_1, int to_close_2){
	pid_t pid, wpid;
	int status = 1;
	
	
	pid = fork();
	if (pid == 0){
		
		if (null_entry){
			redirectIn("/dev/null");
		}
		
		if (fd_in != -1){
			dup2(fd_in,STDIN_FILENO);
			close(fd_in);
		}
		if (fd_out != -1){
			dup2(fd_out,STDOUT_FILENO);
			close(fd_out);
		}
		if (to_close_1 != -1){
			close(to_close_1);
		}
		if (to_close_2 != -1){
			close(to_close_2);
		}
		
		int len = 0;
		for (int k = 0; args[k] != NULL; ++k) {
			len ++;
		}
		char** new_args = (char**)malloc((len + 1) * sizeof(char*));
		int j = 0;
		int if_cont = 0;
		
		for (int i = 0; args[i] != NULL; ++i) {
			if (strcmp(args[i],"if") == 0){
				new_args[j] = args[i];
				if (j == 0 || nsh_if_keyword(args[i-1]) || (i > 1 && nsh_if_keyword_2(args[i-2]))) {
					if_cont++;
				}
				j++;
				continue;
			}
			if (if_cont != 0){
				if (strcmp(args[i],"end") == 0)
					if_cont--;
				new_args[j] = args[i];
				j++;
			} else if (strcmp(args[i], "<") == 0) {
				redirectIn(args[i + 1]);
				i++;
			} else if (strcmp(args[i], ">") == 0) {
				redirectOut(args[i + 1]);
				i++;
			} else if (strcmp(args[i], ">>") == 0) {
				redirectOutAppend(args[i + 1]);
				i++;
			} else {
				new_args[j] = args[i];
				j++;
			}
		}
		
		
		new_args[j] = NULL;
		
//		nsh_print_args(new_args);
		
		for (int i = 0; i < nsh_num_builtin_out(); ++i) {
			if (strcmp(new_args[0], builtin_str_out[i]) == 0){
				exit((*builtin_func_out[i])(new_args));
			}
		}
		
		if (execvp(new_args[0],new_args) != 0){
			free(new_args);
			perror("nsh");
		}
		exit(EXIT_FAILURE);
		
	} else if (pid < 0){
		perror("nsh");
	} else {
		
//		printf("%d\n", pid);
		
		append(to_wait_pid,pid);
		if (fd_in != -1){
			close(fd_in);
		}
		if (fd_out != -1){
			close(fd_out);
		}
		if (should_wait_pipe){
			for (int i = 0; i < to_wait_pid->len; ++i) {
				do {
					int c_pid = to_wait_pid->array[i];
					current_pid_wait = c_pid;
					wpid = waitpid(c_pid, &status , WUNTRACED);
					current_pid_wait = 0;
				} while (!WIFEXITED(status) && !WIFSIGNALED(status));
			}
		}
	}
	
//	printf("%d\n", WEXITSTATUS(status));
	return WEXITSTATUS(status);
}

char** nsh_split_line(char* line){
	
	int bufsize = 64, position = 0;
	char** tokens = malloc(bufsize* sizeof(char*) + 1);
	char* token;
	
	token = strtok(line," \t\r\n\a");
	while(token != NULL){
		
		char* old_token = token;
		
		if (token[0] == '"'){
			if (token[strlen(token) - 1] == '"' && strlen(token) > 1){
				old_token += 1;
				old_token[strlen(old_token) - 1] = 0;
			}
			else{
				old_token = (char*)malloc(strlen(token) * sizeof(char) +1);
				strcpy(old_token,token);
				token = strtok(NULL,"\"");
				old_token = realloc(old_token, (strlen(old_token) + strlen(token) + 1) * sizeof(char));
				old_token = strcat(old_token," ");
				old_token = strcat(old_token,token);
				old_token += 1;
			}
		}
//		else if (token[strlen(token) - 2] == '\\'){
//			old_token = (char*)malloc(strlen(token) * sizeof(char) + 1);
//			strcpy(old_token,token);
//			do {
//				old_token[strlen(old_token) - 1] = 0;
//				old_token[strlen(old_token) - 2] = 0;
//				token = strtok(NULL," \t\r\n\a");
//				old_token = realloc(old_token, (strlen(old_token) + strlen(token) + 1) * sizeof(char));
//				old_token = strcat(old_token," ");
//				old_token = strcat(old_token,token);
//			}while(token[strlen(token) - 1] == '\\');
//		}
		
		
		
		tokens[position] = old_token;
		position++;
		
		if (position >= bufsize){
			bufsize += 64;
			tokens = realloc(tokens,bufsize * sizeof(char*));
		}
		
		token = strtok(NULL," \t\r\n\a");
	}
	
	if (position == 0){
		return NULL;
	}
//	if (strcmp(tokens[position - 1],"&") == 0){
//		should_wait = 0;
//		tokens[--position] = NULL;
//	} else {
//		should_wait = 1;
//		tokens[position] = NULL;
//	}
	tokens[position] = NULL;
	
	return tokens;
}

char* nsh_read_line(long* status){
	char* line = malloc(64);
	size_t bufsize = 64;
	int read = 0;
	char c;
	do{
		if (scanf("%c",&c) == EOF){
			*status = -1;
			return line;
		}
		if (read == bufsize - 1){
			bufsize *= 2;
			line = realloc(line,bufsize);
		}
		line[read++] = c;
	}while(c != '\n');
	line[read] = 0;
	return line;
}

char* strjoin(char** args, char c){
	int len = 0;
	int i;
	for(i = 0; args[i] != NULL; ++i){
		len += (int)strlen(args[i]);
	}
	char* string = malloc(len * sizeof(char) + i * sizeof(char));
	int k = 0;
	for(i = 0; args[i] != NULL; ++i) {
		for (int j = 0; args[i][j] != 0; j++){
			string[k] = args[i][j];
			k++;
		}
		string[k] = c;
		k++;
	}
	string[k-1] = '\n';
	string[k] = 0;
	return string;
}

int nsh_execute(char **args, int should_wait){
	int i;
	
//	nsh_print_args(args);
	
	if (should_wait){
		int k = 0;
		int if_count = 0;
		for (int j = 0; args[j] != NULL; ++j) {
			if (strcmp(args[j], "if") == 0 && (j == k || (j - k > 0 && nsh_if_keyword(args[j-1]))
			|| (j - k > 1 && nsh_if_keyword_2(args[j-2]))))
				if_count ++;
			if (strcmp(args[j], "end") == 0 && if_count > 0)
				if_count --;
			if (if_count == 0){
				if (strcmp(args[j],";") == 0)
				{
					args[j] = 0;
					if (j > 0 && args[j-1] != NULL && strcmp(args[j-1],"&") == 0){
						args[j - 1] = 0;
						nsh_execute(args + k, 0);
						k = j + 1;
						if (args[j+1] == NULL) return 0;
					}
					else if (args[j+1] == NULL){
						return nsh_execute(args + k, 1);
					} else {
						nsh_execute(args + k, 1);
						k = j + 1;
					}
				} if (args[j+1] == NULL){
					if (strcmp(args[j],"&") == 0){
						args[j] = 0;
						nsh_execute(args + k, 0);
						return 0;
					}
					if (k > 0){
						return nsh_execute(args + k, 1);
					}
				}
			}
		}
		if (if_count != 0) {
			fprintf(stderr, "Error parsing, end expected\n");
			return 1;
		}
		if (k != 0) return 0;
	}
	
	
	if (args[0] == NULL)
		return 0;
	
	if (should_wait) {
		for (i = 0; i < nsh_num_builtin(); ++i) {
			if (strcmp(args[0],builtin_str[i]) == 0)
				return (*builtin_func[i])(args);
		}
	}
	
	if (!should_wait){
		int pid;
		should_wait = 1;
		
		pid = fork();
		if(pid == 0){
			setpgid(0,0);
			null_entry = 1;
			int status = nsh_execute(args, 1);
			exit(EXIT_FAILURE);
		}
		else if (pid > 0){
			setpgid(pid,pid);
			append(bg_grp_list,pid);
			append(bg_pid_list,pid);
			appendG(bg_pname_list, strjoin(args, ' '));
			printf("[%d]\t%d\n",bg_grp_list->len,pid);
			return 0;
		}
	}
	
	return nsh_pipe_launch(args);
}

void printPrompt(){
	int capacity = 64;
	char *buf = calloc(capacity,1);
	char*to_free = buf;
	while (getcwd(buf,capacity) == NULL){
		capacity *= 2;
		buf = realloc(buf,capacity);
	}
	fflush(0);
	char hostname[HOST_NAME_MAX];
	gethostname(hostname,HOST_NAME_MAX);
	
	char* home_path = nsh_get_home_adr();
 	int hp_len = (int)strlen(home_path);
 	if (strlen(buf) >= hp_len){
 		char c = buf[hp_len];
 		buf[hp_len] = 0;
 		if (strcmp(buf,home_path) == 0){
		    buf += hp_len - 1;
 			buf[0] = '~';
 			buf[1] = c;
 		} else buf[hp_len] = 0;
 	}
 	free(home_path);
 	
	printf("%s%s@%s%s:%s%s%s\n$ ", BOLD_RED, getlogin(), hostname, WHITE, BOLD_CYAN, buf, WHITE);
	free(to_free);
}

void nsh_load_history(){
	history_list = createListG();
	int status = 0;
	FILE* file = fopen(nsh_history_path,"r+");
	while(status != -1){
		char* line = NULL;
		size_t bufsize = 0;
		status = getline(&line,&bufsize,file);
		if (status == -1)
			continue;
		appendG(history_list,line);
	}
	removeAtIndexG(history_list,history_list->len - 1);
	fclose(file);
}

void nsh_save_history(){
	int nsh_history_fd = open(nsh_history_path,O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
	for (int i = 0; i < history_list->len; ++i) {
		write(nsh_history_fd,getValueAtIndexG(history_list,i),strlen((char*)getValueAtIndexG(history_list,i)));
	}
	write(nsh_history_fd,"\n",1);
	close(nsh_history_fd);
}

void nsh_add_history(char* newline){
	char* to_save = malloc(strlen(newline) + 1);
	strcpy(to_save,newline);
	appendG(history_list,to_save);
	if (history_list->len > 10){
		free(getValueAtIndexG(history_list,0));
		removeAtIndexG(history_list,0);
	}
	nsh_save_history();
}

char* nsh_get_history_command_n(int n){
	char* temp = getValueAtIndexG(history_list,n);
	char* res = malloc((strlen(temp) + 1) * sizeof(char));
	strcpy(res, temp);
	return res;
}

void checkBG(){
	int wpid;
	int stat;
	
	if (bg_pid_list->len > 0){
		int cant_out = 0;
		for (int i = 0; i < bg_pid_list->len; ++i) {
			wpid = waitpid(bg_pid_list->array[i],&stat,WNOHANG);
			if (WIFEXITED(stat)){
				printf("[%d]\tDone\t\t%s",i + 1 + cant_out,(char*)bg_pname_list->array[i]);
				removeAtIndex(bg_pid_list,i);
				removeAtIndexG(bg_pname_list,i);
				removeAtIndex(bg_grp_list,i);
				i = -1;
				cant_out ++;
			}
		}
	}
}
void nsh_loop(){
	char* line;
	char* pline;
	char** args;
	int status = 1;
	long read_status;
	
	do{
		
		ctrc_no_moreline = 0;
		checkBG();
		fflush(0);
		printPrompt();
		fflush(0);
		line = nsh_read_line(&read_status);
		if (read_status < 0)
			break;
		int i = 0;
		char first_char = line[0];
		char* copy = (char*)malloc(strlen(line) + 1);
		strcpy(copy,line);
		char* token = strtok(copy," \n\t\a");
		if (token != NULL && strcmp(token,"again") == 0)
		{
			free(line);
			line = nsh_get_history_command_n(strtol(strtok(NULL," \n\t\a"),0,10) - 1);
			if (line == NULL)
				line = "\n";
		}
		
		
		
		if (first_char != ' ' && first_char != '\n')
			nsh_add_history(line);
		free(copy);
		
//		char* pname = (char*)malloc(strlen(line) + 1);
//		strcpy(pname,line);
		
		pline = nsh_prepare_line(line);
		
		//printf("%s",pline);
		
		args = nsh_split_line(pline);
		if (args == NULL)
			continue;
		
		
//		if (!should_wait)
//		{
//			appendG(bg_pname_list,pname);
//		}
//		else {
//			free(pname);
//		}
		
		status = nsh_execute(args, 1);
		
		
		free(args);
		
		
	}while(1);
}


void ManageCtrC(int signal) {
	if (null_entry){
		int wpid;
		int status;
		int all_done = 1;
		for (int i = 0; i < to_wait_pid->len; ++i) {
			wpid = waitpid(to_wait_pid->array[i],&status,WNOHANG);
			if (WIFEXITED(status) || WIFSIGNALED(status))
				all_done = 0;
		}
		if (all_done)
			exit(1);
		if (kill(current_pid_wait,0) == 0){
			ctrc_no_moreline = 1;
		}
		return;
	}
	else if (current_pid_wait != 0){
		if (!waiting_bg){
			if (kill(current_pid_wait,0) != 0){
				printf("\n");
			}
			else if (current_pid_wait == last_ctr_c_pid){
				for (int i = 0; i < to_wait_pid->len; ++i) {
					kill(to_wait_pid->array[i], SIGKILL);
				}
				printf("\n");
				last_ctr_c_pid = 0;
				ctrc_no_moreline = 1;
				return;
			}
			last_ctr_c_pid = current_pid_wait;
		}
		else {
			if (waiting_bg != last_waiting_bg){
				killpg(waiting_bg,signal);
				last_waiting_bg = waiting_bg;
			}
			else {
				killpg(waiting_bg,SIGKILL);
				printf("\n");
				last_waiting_bg = 0;
			}
		}
	}else{
		printf("\n");
		printPrompt();
		fflush(stdout);
	}
}

void bg_group_start(){
	int pid;
	
	pid = fork();
	if (pid == 0){
		
		setpgid(0,0);
		
		while (1){
			int ppid = getppid();
			if (ppid == 1)
				exit(0);
			if (kill(ppid,0) != 0)
				exit(0);
		}
	} else if ( pid > 0 ){
		bg_group_id = pid;
	}
}


void initialize(){
	//printf("%d\n",getpid());
	setlinebuf(stdin);
	dict_content = createListG();
	dict_keys = createListG();
	to_wait_pid = createList();
	bg_pname_list = createListG();
	bg_pid_list = createList();
	bg_grp_list = createList();
	nsh_set_history_path();
	nsh_load_history();
	signal(SIGINT,ManageCtrC);
}

int main() {
	
//	printf("%d\n", getpid());
	initialize();
	nsh_loop();
	
	return EXIT_SUCCESS;
}
