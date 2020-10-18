//
// Created by daniel on 2020-03-17.
//


#define BOLD_CYAN "\033[1;36m"
#define WHITE "\033[0m"
#define BOLD_RED "\033[1;31m"
#define BOLD_GREEN "\033[1;32m"

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
		if (strcmp(command, "basic") == 0) {
			printf(BOLD_CYAN"1"WHITE". El shell imprime un prompt con la información del usuario logueado, para obtener el "
		  "usuario logueado se usa el metodo "BOLD_GREEN"getlogin()"WHITE", además del directorio donde se encuentra el "
			"shell, donde se sutituye "
			"el path del home por ~, concatenado "BOLD_GREEN"/home"WHITE" con "BOLD_GREEN"getlogin()"WHITE", "
			"seguido de un simbolo de dollar y un espacio.\n"
		  BOLD_CYAN"2"WHITE". Utilizando las funciones "
	BOLD_GREEN"fork"WHITE
	", "BOLD_GREEN"execvp"WHITE" y "BOLD_GREEN"waitpid"WHITE" es posible lograr la ejecución de comandos básicos, haciendo fork "
	"en el shell, con un if preguntamos por el retorno de la funcion fork, si es 0, es que estamos en el proceso hijo "
 "donde procedemos a hacer execpv con los argumentos, en caso contrario hacemos waitpid con el pid que retorna fork, para "
 "esperar a que termine el proceso hijo.\n"
 BOLD_CYAN"3"WHITE". El comando cd es implementado como una función built-in que ejecuta se directamente desde el proceso principal mediante"
 " la función "BOLD_GREEN"chdir"WHITE".\n"
 BOLD_CYAN"4"WHITE". La redireccion de entrada y salida se lleva a cabo seguido del fork, en el proceso hijo antes de hacer exec,"
	" mediante la funcion "BOLD_GREEN"dup2"WHITE", se cambia la salida o entrada estandar al fichero que se pasa como argumento, el fichero se abre solamente "
 "en el proceso hijo con la función "BOLD_GREEN"open"WHITE".\n"
 BOLD_CYAN"5"WHITE". El uso de tuberías se explica con más detalles en el apartado multi-pipe, su funcionamiento está basado en que "
 "antes de hacer fork si se detecta un pipe se abren dos file descriptors con la funcion pipe, y se asigna con dup2, "
 "la salida standar del primer proceso al pipe, y la entrada estandar del segundo proceso al pipe, de esta manera la "
 "salida del primer proceso es la entrada del segundo.\n"
 BOLD_CYAN"6"WHITE". Una vez el comando hijo termina, el padre que hacía wait, continua la ejecución, de esta manera el shell siempre "
 "es un loop que espera por EOF para terminar, y siempre que termine un comando, mostrará el prompt esperando por la "
 "próxima instrucción.\n"
 BOLD_CYAN"7"WHITE". Este punto se explica con más detalle en el apartado spaces, en general se permiten cualquier cantidad de espacios "
 "entre los tokens, al ser separados por un espacio cada uno, se hace un split por espacios y se separa cada token.\n"
 BOLD_CYAN"8"WHITE". Todos los tokens que se encuentren después del token # serán ignorados por el shell. Además si se desea terminar "
 "la ejecución del shell, el comando exit hará que el shell salga del loop antes mencionado y termine.\n"
		  "");
		} else if (strcmp(command, "multi-pipe") == 0) {
			printf("Documentación del multi-pipe\n"
          "El shell permite la concatenación de pipes, una vez tokenizada la entrada, se hace un for que recorre los tokens, "
		  "si el token pipe es encontrado, se procede a crear un array de tamaño 2, sobre el cual se abre el pipe, se ejecutan "
	"las instrucciones a la izquierda del pipe con la entrada redirigida al pipe, y se continua, iterando en el for, los pipes "
 "intermedios que se encuentran proceden de la misma manera, excepto que ahora el proceso de la izquierda tiene su entrada "
 "redirigida desde la salida del ultimo pipe creado, una vez se hace esto, se cierran todos los fd relacionados con el pipe viejo, "
 "y se continua hasta que acaba la linea, el ultimo proceso solo tiene su entrada redirigida desde el ultimo pipe creado. Vale aclarar "
 "que cualquier redirección explicita entre los comandos entre pipes será la ultima redirección que se haga. Mientras estos comandos "
 "se van ejecutando, el shell no hace wait hasta no terminar la linea, cada uno de los pid son añadidos a un lista, para esperar por ellos "
 "al terminar de leer todos los argumentos, de esta manera estos procesos se estarán ejecutando al mismo tiempo.");
		} else if(strcmp(command, "background") == 0){
		    printf("Documentación de background\n"
			 "El shell permite ejecutar procesos en el background mediante el operador &, una vez tokenizada la entrada, si alguna linea "
	"termina en el operador &, eso significará que esta linea se ejecuta en background, para eso, hacemos fork, y desde el proceso hijo "
 "ejecutamos la linea, como mismo lo hariamos, si no estuviera el operador background, de esta manera se grantiza gran compatibilidad "
 "incluso después de hacer cambios o introducir nuevas features, el proceso principal no esperará por el proceso hijo, en cambio añadirá "
 "su pid a una lista de procesos en bg, dicho esto, ahora el proceso padre solo tiene un pid por el que "
 "esperar, al cual espera cada vez antes de imprimir el prompt, con waitpid con el argumento WNOHANG, si este terminó, "
 "se procede a imprimir que el proceso terminó y se remueve de la lista de procesos en background. El comando jobs, nos permite saber que "
 "procesos están en background, esa info se guarda en una lista cada vez que se manda un proceso a bg, y cuando un proceso termina, este "
 "se elimina de la lista. También existe un comando fg <number> que saca al foreground el proceso especificado en number, esto se hace "
 "mediante waitpid, en realidad no se saca al fg, si no que se simula, el proceso principal empieza a esperar por el proceso en bg, y deja "
 "constancia de ello en una variable de estado, que especifica que está esperando por un proceso que se encontraba en background, además de "
 "cuál es su pid; si number no se especifica entonces se saca al fg el último proceso que se mandó al bg . "
 "La entrada de los procesos que van a bg son redirigidas por defecto a /dev/null a menos que se especifique otra entrada.\n");
		}else if(strcmp(command, "spaces") == 0){
            printf("Documentación de spaces\n"
                   "El shell permite el uso de cualquier cantidad de espacios entre comandos y operadores, "
                   "en el parsing, primeramente se añadirá un espacio a cada lado de todos los operadores. Luego para "
				   "tokenizar, se hace con la función strtok, para ignorar multiples espacios, y tratarlos como uno, "
	   "en el shell además se da la posibilidad de unir tokens separados por espacios si se usan comillas dobles entre ellos.\n");
		}else if(strcmp(command, "history") == 0){
            printf("Documentación de history\n"
                   "El shell guarda en un archivo ubicado en /home/user llamado .nsh_history "
				   "todas las entradas que se ejecutan, de esta forma al "
                   "pedirle history es capaz de mostrar los últimos 10 enumerados debidamente y garantiza persistencia. "
				   "Si se quiere que una entrada no se guarde en el history entonces basta con poner al menos un espacio "
	   "como el primer caracter de la entrada.\n"
                   "El comando again <number>, ejecutará la entrada guardada en el history bajo ese número, en nuestro shell, "
				   "este comando no tiene interacción con otras funcionalidades, solo se permite al comienzo de una entrada "
	   "y se ignora lo demás que le siga, si no hay espacios antes de again entonces, se guardara en el history como el "
	"comando por el que se sustituyó y no por again.\n");
        }else if(strcmp(command, "ctrl+c") == 0){
		    printf("Documentación de ctrl+c\n"
             "Mediante el uso de la función signal, el shell es capaz de capturar SIGINT y ejecutar una función para "
             "así actuar de manera debida, primeramente, el shell no se cerrará con la señal SIGINT, además si los procesos "
			 "por los que espera no se cerraron al mandarle SIGINT, entonces, la próxima vez le enviaremos la señal SIGKILL "
	"para terminar esos procesos de manera forzada.\n"
 "En el caso de los procesos que se encuentran en bg, al estar en un grupo diferente no reciben la señal. En cuanto a los "
 "procesos que están en bg y fueron enviados al fg, como no los cambiamos de grupo, directamente enviamos la señala ellos, como "
 "anteriormente mencionamos, al esperar por un proceso que estaba en background dejabamos constancia de ello en un status, así"
 "podemos diferenciar las dos situaciones, como el proceso en bg tiene el mismo signal handler que el shell principal, este "
 "tiene un flag que lo diferencia y hará que su comportamiento sea diferente, en este caso, no morirá, y si le llega la señal SIGINT,"
 " la propagará por sus hijos, y si le vuelve a llegar (solo posible si no termino la primera vez), entonces propagará SIGKILL a sus hijos.");
		}else if(strcmp(command, "chain") == 0){
            printf("Documentación de chain\n"
				   "El comando "BOLD_GREEN";"WHITE" en nuestro proyecto es como un separador de líneas por lo cual se permite cualquier operacion anterior "
	   "entre "BOLD_GREEN";"WHITE" que debe tener el comportamiento esperado, solo el comando again que como anteriormente explicamos, no "
	"le dimos interacción con los demás comandos del shell, aunque bien again puede repetir un comando con varias lineas "
 "o en general cualquier comando anteriormente usado. Para lograr el funcionamiento de este operador, después de tokenizar, y antes de cualquier "
 "otra cosa, procedemos a separar la entrada en líneas, y a ejecutarlas una a una, mientras las separamos. Cabe añadir que "
 "la función "BOLD_GREEN"if"WHITE" del shell, es una función multilínea y por lo tanto se ejecutará en su conjunto y sus líneas no serán separadas "
	"en este punto, el comportamiento del if está explicado en su apartado. Si alguna linea lleva bg no se espera por esta para ejecutar la "
 "siguiente línea, en caso contrario si y se ejecutan en orden.\n"
				   "Los otros operadores chain se hacen de manera muy similar al operador pipe, estos serían "BOLD_GREEN"||"WHITE" y "
  BOLD_GREEN"&&"WHITE" conocidos como or y and respectivamente, estos comandos se encuentran adentro de las líneas, y una vez se tiene "
					 "la línea tokenizada, se hace un for buscando estos operadores y pipe, el comportamiento de pipe fue explicado en su "
	  "apartado, si es un encontrado or o and, entonces se espera por los comandos que se encuentran a su izquierda, que ya se estaban ejecutando, "
   "y se manda a ejecutar el comando inmediato a su izquierda por el cual también se espera, el exit status de este conjunto de comandos de la "
   "izquierda, que pueden estar unidos por varios operadores or o and, incluso pipes, en caso de pipe se ignora el exit status de los comandos "
   "a la izquierda, en caso de and, es un and lógico entre los exit status de los comandos a su izquierda y derecha, y el or es un or lógico "
   "entre los comandos a su izquierda y derecha, como el valor se arrastra de izquierda a derecha, en cada momento solo tenemos el valor de la izquierda "
   "del operador, el exit status del primer comando de la derecha sería la parte derecha del operador, se guarda para ello cual fue el último "
   "operador encontrado y el exit status del argumento a su izquierda, y se continúa iterando en busqueda de otros operadores de chain o bien el final de la línea"
   ", así se respeta el orden en que aparecen los operadores y no se permite agrupación de comandos.\n");
        }else if(strcmp(command, "if") == 0){
            printf("Documentación de if\n"
				   "El comando if es implementado como un built-in del shell, como se había mencionado es un comando multilínea, aunque bien "
	   "puede escribirse en una sola línea, es un built-in con fork que permite redirección e incluso el uso de pipes, and y or para relacionarlo "
	"con otros comandos, el comando utiliza el mismo método que el shell utiliza para ejecutar una entrada completa tokenizada, "
 "para ejecutar el conjunto de comandos condición, y en dependencia de su exit status entonces procede a ejecutar then o else con el mismo método, los cuales pueden "
 "o no estar presentes, en caso de no estar presentes su exit status es 0 y se considera como una entrada vacía. También se permite explícitamente "
 "dejar vacíos los comandos condición, then o else, en este caso su exit status es 0. El exit status del if en general está dado "
 "por el exit status del then o else, en dependencia, de si el exit status de la condición fue 0 o no respectivamente. El comando if, en nuestro shell,"
 " a pesar de usar el mismo método para ejecutar sus statements que el que usa el shell, no permitirá bg en sus statements (sí de forma general), aunque se puede poner y se realizará como es esperado,"
 " pasa que por esos comandos nunca se esperan, por lo cual no es recomdable su uso, esto nos pasó por la forma en que decidimos enfrentar el problema, "
 "ya que if se ejecuta en un hijo, no le pasa la información de los procesos que crea y no espera, al proceso principal. Es importante aclarar "
 "que a lo hora de parsear se ignora todo lo que se encuentre dentro de un if y se deja al if resolver ese problema.\n"
 "Además de estos comandos se han implementado los comandos "BOLD_GREEN"true"WHITE" y "BOLD_GREEN"false"WHITE" tal que terminan con exit "
	"status 0 y 1 respectivamente.\n");
        }else if(strcmp(command, "multi-if") == 0){
            printf("Documentación de multi-if\n"
				   "Con el apartado if anteriormente explicado, queda bastante claro que los multi-if son naturalemnte permitidos, "
	   "ya que if ejecuta sus statements como si fueran una línea completa, no hay problema para que ejecute un if dentro de si mismo. El cambio "
	"sería a lo hora de parsear, donde se busca el balanceo entre if y ends para saber cuando nos encontramos dentro de los statements de un if.\n");
        }else if(strcmp(command, "variables") == 0){
            printf("Documentación de variables\n"
				   "Las variables en el proyecto son guardadas en dos listas con relación llave, valor entre elementos con "
	   "el mismo índice en las dos lístas.\n"
	"La función set nos permite ver los valores asignados a cada llave\n"
 "La función set también puede ser usada para asignarle un valor a una llave de la siguiente manera, set <llave> <valor>, "
 "todo lo que se encuentre despúes del token llave se añade como valor, excepto por los comentarios. Si la llave no existe se crea, "
 "y en caso de existir ya su valor es sustituido por el nuevo valor. Otro uso viene dado por las comillas invertidas, el comando "
 "set <llave> `comando`, guarda en como valor de la llave la salida del comando, para esto se hace un pipe y fork y se pone a un shell hijo "
 "a ejecutar la linea entre comillas invertidas con salida al pipe, luego de que este termina se lee el pipe y se copia lo que se lee "
 "al valor de la llave. Este comando, como el comando again, tampoco tiene interacción con las otras funcionalidades del shell, debe usarse "
 "como único comando de la entrada. Todos los comandos funcionan dentro de las comillas invertidas, salvo, aquellos comandos que requieren "
 "que sean ejecutados directamente en el shell padre para que su comportamiento tenga efecto, ponemos de ejemplo cd, again, o el mismo set.\n"
 "La función unset <llave>, elimina la llave y su valor de las listas. Es importante aclarar que si no se ejecuta directamente en el shell padre, "
 "no tendrá ningún efecto.\n"
 "La función get <llave> podrá usarse en cualquier contexto e imprime el valor de la llave que se pide. En caso de que no esté la llave se "
 "imprime que la llave no fue encontrada.\n");
        }else if(strcmp(command, "help") == 0){
			printf("Documentación de help\n"
			       "Help es el comando de ayuda de nuestro proyecto, su modo de uso es help <command>, o bien help para mostrar "
		  "los comandos con ayuda disponible. Help está implementado como un built-in con redirección e interacción completa "
	"con las demás funciones.\n");
		}
		else {
			printf("Funcionalidad no reconocida, escriba help para acceder a las funcionalidades disponibles\n");
		}
	} else {
		printf("Integrantes: Daniel Rivero Gómez-Wangüemert & Javier Rodriguez Rodriguez\n"
		 "Este es el comando de ayuda de nuestro proyecto\n\n"
   "Modo de uso: help <comando>\n\n"
   "Comandos:\n\n"
   BOLD_GREEN"basic"WHITE":\t\t funcionalidades básicas ("BOLD_CYAN"3"WHITE" puntos)\n"
   BOLD_GREEN"multi-pipe"WHITE":\t múltiples tuberías ("BOLD_CYAN"1"WHITE" punto)\n"
   BOLD_GREEN"background"WHITE":\t comandos en background ("BOLD_CYAN"0.5"WHITE" puntos)\n"
   BOLD_GREEN"spaces"WHITE":\t\t espacios variables entre operadores ("BOLD_CYAN"0.5"WHITE" puntos)\n"
   BOLD_GREEN"history"WHITE":\t historial de comandos usados ("BOLD_CYAN"0.5"WHITE" puntos)\n"
   BOLD_GREEN"ctrl+c"WHITE":\t\t capturar ctrl+c ("BOLD_CYAN"0.5"WHITE" puntos)\n"
   BOLD_GREEN"chain"WHITE":\t\t operadores para encadenar comandos ("BOLD_CYAN"0.5"WHITE" puntos)\n"
   BOLD_GREEN"if"WHITE":\t\t operador condicional ("BOLD_CYAN"1"WHITE" punto)\n"
   BOLD_GREEN"multi-if"WHITE":\t if anidados ("BOLD_CYAN"0.5"WHITE" puntos)\n"
   BOLD_GREEN"help"WHITE":\t\t comando de ayuda ("BOLD_CYAN"1"WHITE" punto)\n"
   BOLD_GREEN"svariables"WHITE":\t almacenar e imprimir variables ("BOLD_CYAN"1"WHITE" punto)\n"
   "\n"
   "Total:\t "BOLD_CYAN"10"WHITE" puntos\n");
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