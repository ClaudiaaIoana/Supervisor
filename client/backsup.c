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
#include <sys/resource.h>
#include<sys/stat.h>
#include<sys/types.h>

    //when working with pipes
#define READ_END    0
#define WRITE_END   1

    //when working with standard fd
#define FD_STDIN    0
#define FD_STDOUT   1
#define FD_STDERR   2


void *manager_thread(void* param);

void analise_conf(char * file_name);

void create_child_proccess(char* file_name,int uid,int gid);

void redirect(char* parameters, int fd_redirected);

void set_ownership(int uid,int gid);

void change_priority(int new_priority);

void change_umask(mode_t new_umask );

void change_exec_directory(char* path_to_directory);

 
int main()
{
    //redirect("out.txt create append",FD_STDOUT);
    create_child_proccess("./test1",7878,7878);

    int status;
    pid_t wpid;

    // Wait for all child processes to finish
    while ((wpid = wait(&status)) > 0) {
        if (WIFEXITED(status)) {
            printf("Child process %d (PID: %d) exited with status %d\n", wpid, getpid(), WEXITSTATUS(status));
        } else {
            printf("Child process %d (PID: %d) terminated abnormally\n", wpid, getpid());
        }
    }
 
	return 0;
}

/////////////////////////////
//                              DEFINE FUNCTIONS
/////////////////////////////

        //Create the child procces, modify the parameters and lounch the executable
void create_child_proccess(char* exec_name,int uid,int gid)
{
    pid_t pid;
	pid = fork();
    if(pid<0)
    {
        perror("Counldn't create child process");
    }
	if (pid == 0) 
    {
        printf("Child proccess PID %d\n",getpid());

	    //execlp(exec_name,exec_name,NULL);
        exit(EXIT_SUCCESS);
	}
	if (pid > 0)
	{
        printf("Parent proccess PID %d\n",getpid());
    }
}


        //redirects the stdin, stdout, stderr to the specified file
void redirect(char* parameters, int fd_redirected)
{
    char copy[50];
    strcpy(copy,parameters);

    char*filename=strtok(copy," :");
    int flags= (int)(fd_redirected>=1);
    char*flag=strtok(NULL," \n\0");

    while(flag)
    {
        if(strcmp(flag,"create")==0)
        {
            flags |=O_CREAT;
        }
        else if (strcmp(flag,"truncate")==0)
        {
            flags|=O_TRUNC;
        }
        else if(strcmp(flag,"append")==0)
        {
            flags|=O_APPEND;
        }
        flag=strtok(NULL," \n\0");
    }
    int fd_out = open(filename, flags, 0644);
    dup2(fd_out,fd_redirected);
    close(fd_out);
}


        //change the owners of the proccess, first the group_owner, lather the owner
        //if we change first the user then it may not have enought permitions to change the group_owner
        //this function has to be called right before the exec command otherwise the rest of the parameter changes may not be done 
void set_ownership(int uid,int gid)
{
    if (setgid(gid) != 0) {
        perror("Error setting GID");
        exit(EXIT_FAILURE);
    }

    if (setuid(uid) != 0) {
    perror("Error setting UID");
    exit(EXIT_FAILURE);
    }
}

        //change the priority of the proccess
void change_priority(int new_priority)
{
    int which = PRIO_PROCESS;  // Adjust the priority of a process
    id_t pid = getpid();        

    if (setpriority(which, pid, new_priority) == -1) 
    {
        perror("Error setting process priority");
        exit(EXIT_FAILURE);
    }
}

void change_umask(mode_t new_umask)
{
    umask(new_umask);
}

void change_exec_directory(char* path_to_directory)
{
     if (chdir(path_to_directory) != 0) 
     {
        perror("chdir failed");
        exit(EXIT_FAILURE);
    }
}
