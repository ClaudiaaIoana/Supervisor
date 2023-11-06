#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <pthread.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <signal.h>

#define READ_END 0
#define WRITE_END 1
//int program_active=1;

char message[100]="";
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

struct parameters {
    int pid;
};
typedef struct parameters parameters_t;


void *manager_thread(void* param)
{
    parameters_t* param_=(parameters_t*) param;
    int n=0;
    while(1)
    {   
        printf("Asteapta mesaj\n");
        //pthread_mutex_lock(&mutex);
        while (strcmp(message,"")==0) 
        {
            pthread_cond_wait(&cond, &mutex);
        }
        
    
        printf("A primit mesaj %s\n",message);
        int fd_red=open(message,O_WRONLY | O_CREAT, 640);

        printf("A deschis fisier\n");
        struct user_regs_struct regs;

        kill(param_->pid, SIGSTOP);
        ptrace(PTRACE_ATTACH, param_->pid, NULL, NULL);
        //waitpid(param_->pid, NULL, 0);
        ptrace(PTRACE_GETREGS, param_->pid, NULL, &regs);

        regs.rdi = fd_red;

        ptrace(PTRACE_SETREGS, param_->pid, NULL, &regs);
        ptrace(PTRACE_DETACH, param_->pid, NULL, NULL);
        kill(param_->pid, SIGCONT);

        printf("A redirectat si a resetat message\n");

        strcpy(message,"");
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

void create_child_proccess(char* exec_name)
{
    pid_t pid;
	pid = fork();
    if(pid<0)
    {
        perror("Counldn't create child process");
    }
	if (pid == 0) 
    {
        printf("Created child\n");

        int fd=open("out.txt", O_WRONLY);
        dup2(fd,1);
        close(fd);

	    execlp(exec_name,exec_name,NULL);

	}
	if (pid > 0)
	{
        printf("Parent proccess \n");
        //printf("Creare thread \n");

        //create_thread(pipe_fd[READ_END]);
        //pthread_t tid;
        //parameters_t param;
        //param.pid=pid;
        //pthread_create(&tid, NULL, manager_thread , &param);
        //pthread_mutex_lock(&mutex);
    }
}
 
int main()
{
	
    create_child_proccess("./test");
    //sleep(5);

    //printf("Mutex created\n");
    //strcpy(message,"out.txt");
    //printf("Message: %s",message);
    //pthread_cond_signal(&cond);
    //pthread_mutex_unlock(&mutex);
 
	return 0;
}