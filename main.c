/* Zachary Smith & Sam Whalen
**OS Project 2*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <poll.h>
#include <signal.h>
#include <errno.h>
	 
	struct node {
		char *name;
		struct node *next;
	};
	 
	char **toks(const char *str) {//count tokens
	        int num_tokens = 0;
	        const char *space = " \n\t";
	        char *tmp;
		char *word;
	        char *s = strdup(str);
	        for ( word = strtok_r(s, space, &tmp); word != NULL ; word = strtok_r(NULL, space, &tmp) ) {
	                num_tokens++;
	        }
	        free(s);
	 
	        char **token_list = (char**) malloc(sizeof(char*)*(num_tokens+1)); // create array
	 
	        s = strdup(str);
	        int i = 0;
	        for ( word = strtok_r(s, space, &tmp); word != NULL ; word = strtok_r(NULL, space, &tmp) ) { // add toks
	                char *token = strdup(word);
	                token_list[i] = token;
	                i++;
	        }
	        token_list[i] = NULL;
	        free(s);
	 
	        return token_list;
	}
	 
	char ***tokenify(const char *str, int commands) {
		char ***cmdtokens = NULL;
		if (commands <= 0 ) {
			return cmdtokens;
		}
		cmdtokens = (char***) malloc(sizeof(char**)*(commands+1));
	 
		const char *space = ";";
		char *tmp, *word;
		char *s = strdup(str);
		word = strtok_r(s, space, &tmp);
	 
		int i=0;
		for (; i < commands; i++) {
			if (word != NULL) {
				cmdtokens[i] = toks(word);// tokenize & add
			} else {
				cmdtokens[i] = (char**)malloc(sizeof(char*));
				cmdtokens[i][0] = '\0';
			}
			word = strtok_r(NULL, space, &tmp);
		}
	 
		free(s);
		cmdtokens[i] = NULL;
		return cmdtokens;
	}
	 
	char *strip(char *str) {
		int len = strlen(str);
		if (str[len-1] == (int)'\n')
			str[len-1] = '\0';
		return str;
		printf("\n%s\n",str);
	}
	 
	int main(int argc, char **argv) {
		char buff[1024];
		int par=0;
		int p_running=0;
		int childrv;
		int exit=0;
		pid_t pid=1;
	 
		FILE *configfile = fopen("shell-config","r");  //shell-config
		struct node *head = NULL;
		struct node *tmp;
	 
		if (configfile != NULL) {
	                char fbuff[256];
	                head = (struct node*) malloc(sizeof(struct node));
	                head->name = strdup(strip(fgets(fbuff, 256, configfile)));
	                head->next = NULL;
	 
	                while (fgets(fbuff, 256, configfile) != NULL) {
	                        tmp = (struct node*) malloc(sizeof(struct node));
	                        tmp->name = strip(strdup(fbuff));
	                        tmp->next = head;
	                        head = tmp;
	                }
	        }
	 
		fclose(configfile);
	 
		printf("Please type input here: ");
		fflush(stdout);
	 
		while (fgets(buff, 1024, stdin) != NULL) { // count commands
			int commands, i, j;
			commands=1;
			for (i=0; i<1024; i++) {
				if ( buff[i] == (int)'\0' )
					break;
				if ( buff[i] == (int)';' )
					commands++;
				if ( buff[i] == (int)'#' ) {
					buff[i]='\0';
					break;
				}
			}
	 
			// tokenize commands and organize into array of array of pointers
			char ***commandtoks = tokenify(&buff, commands);
	 
	 
			// execute commands
			i = 0; 
			j = 0;
			
			while (commandtoks[i]!=NULL) {
				if (commandtoks[i][0]==NULL){
					break;
	 			}
				
				if (commandtoks[i][0][0]==4) {// EOF
					printf("\n");
					break;
				}
	 
				if (strcmp(commandtoks[i][0],"mode")==0) { //modes
					if (commandtoks[i][1] == NULL) {
						if (par) {
							printf("current mode: parallel\n");
						} else {
							printf("current mode: sequential\n");
						}
					} else if (commandtoks[i][2] == NULL) {
						if (strcmp(commandtoks[i][1], "sequential")==0 || strcmp(commandtoks[i][1], "s")==0) {
							if (par==1) {
								
	                        	while (p_running > 0) {
	                            waitpid(-1, &childrv, 0);
	                            p_running--;
					            }
							}
							par=0;
						} else if (strcmp(commandtoks[i][1], "parallel")==0 || strcmp(commandtoks[i][1], "p")==0) {
							par=1;
						} else {
							fprintf(stderr,"Mode error: invalid command. Invalid mode parameter specified\n");
						}
					} else {
						fprintf(stderr,"Mode error: invalid command. Mode does not take more than one parameter\n");
					}
				}
	 
				else if(strcmp(commandtoks[i][0],"exit")==0) { //exit
					if (commandtoks[i][1] == NULL) {
						exit=1;
						if (!par)
							break;
					} else {
						fprintf(stderr,"Error exiting: invalid command. Exit does not take any parameters\n");
					}
				}
	 
				else { //commands
					pid = fork();
					p_running++;
	 
					if (pid==0) {
						int skip=0;
						tmp = head;
						while (tmp!=NULL) {
							int size = strlen(tmp->name)+strlen(commandtoks[i][0])+2;
							char *path = (char*) malloc(size);
							strcpy(path, tmp->name);
							strcat(path, "/");
							strcat(path, commandtoks[i][0]);
							strcat(path, "\0");
	 
							if (execv(path, commandtoks[i]) != -1) {
								skip=1;
								free(path);
								break;
							}
	 
							free(path);
							tmp = tmp->next;
						}
						if (!skip) {
							if (execv(commandtoks[i][0], commandtoks[i]) < 0) {
								fprintf(stderr, "execv failed: %s\n", strerror(errno));
							}
						}
						return 0;
					} else {
						if (!par) {
							// waiting
	                    	waitpid(pid, &childrv, 0);
							p_running--;
						}
					}
				}
	 
				i++;
			}
	 
			if (par) {
				while (p_running > 0) { //waiting
					waitpid(-1, &childrv, 0);
					p_running--;
				}
			}
	 
			if (commandtoks != NULL) { //free
				i=0;
			    	j=0;
				while (commandtoks[i]!=NULL) {
					j=0;
					while (commandtoks[i][j]!='\0') {
						free(commandtoks[i][j]);
						j++;
					}
					free(commandtoks[i]);
					i++;
				}
				free(commandtoks);
			}
	 
			if (exit)
				break;
	 
			printf("Please type input here: ");
			fflush(stdout);
	 
		}
	 
		while (head!=NULL) { //free heap mem
			tmp = head;
			head = head->next;
			if (tmp->name != NULL) {
				free(tmp->name);
			}
			free(tmp);
		}
	 
	    	return 0;
	}



