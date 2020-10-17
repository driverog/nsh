//
// Created by daniel on 2020-03-17.
//


#include "builtin.h"


char* builtin_str[5] = {
"cd",
"exit",
"fg",
"set",
"unset"
};

char* builtin_str_out[7] = {
		"jobs",
		"history",
		"true",
		"false",
		"if",
		"help",
		"get"
};

int (*builtin_func[]) (char**) = {
		&nsh_cd,
		&nsh_exit,
		&nsh_foreground,
		&nsh_set,
		&nsh_unset
};

int (*builtin_func_out[]) (char**) = {
		&nsh_jobs,
		&nsh_print_history,
		&nsh_true,
		&nsh_false,
		&nsh_if,
		&nsh_help,
		&nsh_get
};

int nsh_num_builtin(){
	return sizeof(builtin_str) / sizeof(char*);
}

int nsh_num_builtin_out(){
	return sizeof(builtin_str_out) / sizeof(char*);
}

int nsh_cd(char** args){
	if (args[1] != NULL){
		if (chdir(args[1]) != 0)
			perror("nsh: cd");
	}
	return 0;
}

int nsh_jobs(char** args){
	for (int i = 0; i < bg_pid_list->len; ++i) {
		printf("[%d]\t%s",i+1,(char*)getValueAtIndexG(bg_pname_list,i));
	}
	return 0;
}

int nsh_foreground(char** args){
	if (bg_pid_list->len == 0)
		return 1;
	
	int cpid;
	int status;
	int wpid;
	
	int index = args[1] == NULL ? bg_pid_list->len - 1 : (int)strtol(args[1], 0, 10) - 1;
	if (index >= bg_pid_list->len)
		return 2;
	cpid = bg_pid_list->array[index];
	waiting_bg = cpid;
	do {
		current_pid_wait = cpid;
		wpid = waitpid(cpid, &status,WUNTRACED);
		current_pid_wait = 0;
	} while (!WIFEXITED(status) && !WIFSIGNALED(status));
	waiting_bg = 0;
	removeAtIndex(bg_pid_list,index);
	removeAtIndexG(bg_pname_list,index);
	removeAtIndex(bg_grp_list,index);
	
	return 0;
}

int nsh_exit(char** args){
	exit(EXIT_SUCCESS);
}

char* nsh_get_home_adr(){
	int len = (int)strlen(getlogin());
	char* path = calloc((strlen("/home/") + len + 1) * sizeof(char), sizeof(char));
	strcpy(path,"/home/");
	strcat(path,getlogin());
	return path;
}

void nsh_set_history_path(){
	char* home_path = nsh_get_home_adr();
	char* fd_path = calloc((strlen(home_path) + strlen("/.nsh_history"))* sizeof(char), sizeof(char));
	strcpy(fd_path,home_path);
	strcat(fd_path,"/.nsh_history");
	nsh_history_path = fd_path;
	close(open(fd_path,O_WRONLY | O_CREAT,S_IRWXU));
	free(home_path);
}

int nsh_print_history(char** args){
	for (int i = 0; i < history_list->len; ++i) {
		printf("%d:\t%s",i+1,(char*)getValueAtIndexG(history_list,i));
	}
	return 0;
}

int nsh_true(char** args){
	return EXIT_SUCCESS;
}

int nsh_false(char** args){
	return EXIT_FAILURE;
}

int nsh_if_keyword(char* word) {
	if (word == NULL) {
		return 1;
	}
	return strcmp(word, "if") == 0 || strcmp(word, "then") == 0 || strcmp(word, "else") == 0
	|| strcmp(word, "|") == 0 || strcmp(word, "&&") == 0 || strcmp(word, "||") == 0 || strcmp(word, ";") == 0;
}

int nsh_if_keyword_2(char* word) {
	if (word == NULL) {
		return 0;
	}
	return strcmp(word, ">") == 0 || strcmp(word, ">>") == 0 || strcmp(word, "<") == 0;
}

void nsh_print_args(char** args) {
	for (int i = 0; args[i] != NULL; ++i) {
		printf("%s ", args[i]);
	}
	printf("\n");
}

int nsh_if(char** args){
//	args[0] = 0;
	int then = 0;
	int els = 0;
	int if_cont = 0;
//	nsh_print_args(args);
	for (int j = 1; args[j] != NULL; ++j) {
		if (strcmp(args[j],"if") == 0 && (nsh_if_keyword(args[j-1]) || (j > 1 && nsh_if_keyword_2(args[j-2])))){
			if_cont++;
		}
		else if (strcmp(args[j],"then") == 0 && if_cont == 0){
			args[j] = 0;
			then = j + 1;
		}
		else if (strcmp(args[j],"else") == 0 && if_cont == 0){
			args[j] = 0;
			els = j + 1;
		}
		else if (strcmp(args[j],"end") == 0){
			if (if_cont == 0) args[j] = 0;
			else if_cont--;
		}
	}
//	nsh_print_args(args);
	if (nsh_execute(args + 1, 1) == 0){
		if (then == 0)
			return 0;
		return nsh_execute(args + then, 1);
	} else {
		if (els == 0)
			return 0;
		return nsh_execute(args + els, 1);
	}
}

int nsh_help(char** args){
	if (args[1] != NULL){
		char* command = args[1];
		if (strcmp(command, "help") == 0) {
			printf("help es el comando de ayuda de nuestro proyecto, está implementado como una"
		  " funcionalidad built-in que admite redireccion de entrada y salida, y el uso junto a otros "
	"comandos con operadores de cadena.\n"
	"El uso de este comando mostrará el conjunto de funcionalidades de las que dispone este proyecto, "
 "además de una explicación de su implementación\n"
		  "");
		} else if (strcmp(command, "basic") == 0) {
			printf("1. El shell imprime un prompt con la información del usuario logueado, además del "
			       "directorio donde se encuentra el shell, seguido de un simbolo de dollar y un espacio.\n"
		  "2. Utilizando las funciones fork, exec y wait es posible lograr la ejecución de comandos básicos.\n"
 "3. El comando cd es implementado como una función built-in que ejecuta se directamente desde el proceso principal mediante"
 " la función chdir.\n"
 "4. La redireccion de entrada y salida se lleva a cabo seguido del fork, en el proceso hijo antes de hacer exec,"
 " mediante la funcion dup2, se cambia la salida o entrada estandar al fichero que se pasa como argumento.\n"
 "5. El uso de tuberías se explica con más detalles en el apartado multi-pipe, su funcionamiento está basado en que "
 "antes de hacer fork si se detecta un pipe se abren dos file descriptors con la funcion pipe, y se asigna con dup2, "
 "la salida standar del primer proceso al pipe, y la entrada estandar del segundo proceso al pipe, de esta manera la "
 "salida del primer proceso es la entrada del segundo.\n"
 "6. Una vez el comando hijo termina, el padre que hacía wait, continua la ejecución, de esta manera el shell siempre "
 "es un loop que espera por EOF para terminar, y siempre que termine un comando, mostrará el prompt esperando por la "
 "próxima instrucción.\n"
 "7. Este punto se explica con más detalle en el apartado spaces, en general se permiten cualquier cantidad de espacios "
 "entre los tokens, al ser separados por un espacio cada uno, se hace un split por espacios y se separa cada token.\n"
 "8. Todos los tokens que se encuentren después del token # serán ignorados por el shell. Además si se desea terminar "
 "la ejecución del shell, el comando exit hará que el shell salga del loop antes mencionado y termine.\n"
		  "");
		} else if (strcmp(command, "multi-pipe") == 0) {
			printf("Documentación del multi-pipe\n");
		} else {
			printf("Funcionalidad no reconocida, escriba help para acceder a las funcionalidades disponibles\n");
		}
	} else {
		printf("Integrantes: Daniel Rivero Gómez-Wangüemert & Javier Rodriguez Rodriguez\n"
		 "Este es el comando de ayuda de nuestro proyecto\n\n"
   "Modo de uso: help <comando>\n\n"
   "Comandos:\n\n"
   "basic:\t\t funcionalidades básicas (3 puntos)\n"
   "multi-pipe:\t múltiples tuberías (1 punto)\n"
   "background:\t comandos en background (0.5 puntos)\n"
   "spaces:\t\t espacios variables entre operadores (0.5 puntos)\n"
   "history:\t historial de comandos usados (0.5 puntos)\n"
   "ctrl-c:\t\t capturar ctrl+c (0.5 puntos)\n"
   "chain:\t\t operadores para encadenar comandos (0.5 puntos)\n"
   "if:\t\t operador condicional (1 punto)\n"
   "multi-if:\t if anidados (0.5 puntos)\n"
   "help:\t\t comando de ayuda (1 punto)\n"
   "variables:\t almacenar e imprimir variables (1 punto)\n"
   "\n"
   "Total:\t 10 puntos\n");
	}
	return 0;
}

int nsh_set(char** args){
	if (args[1] == NULL){
		for (int i = 0; i < dict_keys->len; ++i) {
			printf("%s=%s\n",(char*)dict_keys->array[i],(char*)dict_content->array[i]);
		}
	} else {
		int index = -1;
		for (int i = 0; i < dict_keys->len; ++i) {
			if (strcmp(args[1],dict_keys->array[i]) == 0) index = i; break;
		}
		char* str = malloc(1);
		str[0] = 0;
		if (args[2] == NULL)
			str[0] = 0;
		else if (args[2][0] == '`') {
			free(str);
			int len;
			for (len = 0; args[len] != NULL ; ++len) {}
			args[2] += 1;
			args[len - 1][strlen(args[len - 1]) - 1] = 0;
			
			int fd[2];
			int pid;
			pipe(fd);
			
			pid = fork();
			if (pid == 0){
				dup2(fd[1],STDOUT_FILENO);
				close(fd[1]);
				close(fd[0]);
				nsh_execute(args + 2, 1);
				exit(0);
			} else if (pid > 0){
				int status;
				int wpid;
				do {
					current_pid_wait = pid;
					wpid = waitpid(pid, &status , WUNTRACED);
					current_pid_wait = 0;
				} while (!WIFEXITED(status) && !WIFSIGNALED(status));
				
				close(fd[1]);
				
				char c;
				int bufsize = 64, i = 0;
				str = malloc(bufsize);
				while (read(fd[0],&c,1) > 0){
					str[i] = c;
					i++;
					if (i >= bufsize){
						str = realloc(str,bufsize * 2);
						bufsize *= 2;
					}
				}
				str[i] = 0;
				if (str[i - 1] == '\n')
					str[i - 1] = 0;
				close(fd[0]);
			}
		} else {
			for (int i = 2; args[i] != NULL; ++i) {
				str = realloc(str,strlen(str) + strlen(args[i]) + 2);
				if (i != 2)
					strcat(str," ");
				strcat(str,args[i]);
			}
		}
		
		if (index == -1){
			char* key = malloc(strlen(args[1]) + 1);
			strcpy(key,args[1]);
			appendG(dict_keys,key);
			appendG(dict_content,str);
		} else {
			setValueAtIndexG(dict_content,index,str);
		}
	}
	
	return 0;
}

int nsh_unset(char** args){
	if (args[1] == NULL)
		return 0;
	
	for (int i = 0; i < dict_keys->len; ++i) {
		if (strcmp(args[1],dict_keys->array[i]) == 0){
			removeAtIndexG(dict_keys,i);
			removeAtIndexG(dict_content,i);
			break;
		}
	}
	
	return 0;
}

int nsh_get(char** args){
	if (args[1] == NULL)
		return 0;
	
	int found = 0;
	for (int i = 0; i < dict_keys->len; ++i) {
		if (strcmp(args[1],dict_keys->array[i]) == 0){
			printf("%s\n",(char*)dict_content->array[i]);
			found = 1;
			break;
		}
	}
	
	if (found == 0){
		write(2,"get: variable not found\n",24);
	}
	
	return 0;
}