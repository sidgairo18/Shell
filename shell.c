#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <termios.h>

//Global variables
char hostname[255];
char username[255];
char cwd[255];
char user[1024];

typedef struct b{
    pid_t p;
    char name[100];
}b;

b jobs[100000];
int jobsn=0;


pid_t pid;
int status;


pid_t shell_pgid;
struct termios shell_tmodes;
int shell_terminal;
int shell_is_interactive;



void init_shell ()
{

    //See if we are running interactively.
    shell_terminal = STDIN_FILENO;
    shell_is_interactive = isatty (shell_terminal);

    if (shell_is_interactive)
    {
        // Loop until we are in the foreground.
        while (tcgetpgrp (shell_terminal) != (shell_pgid = getpgrp ()))
            kill (- shell_pgid, SIGTTIN);

        // Ignore interactive and job-control signals.
        signal (SIGINT, SIG_IGN);
        signal (SIGQUIT, SIG_IGN);
        signal (SIGTSTP, SIG_IGN);
        signal (SIGTTIN, SIG_IGN);
        signal (SIGTTOU, SIG_IGN);
        signal (SIGCHLD, SIG_IGN);

        // Putting in own process group
        shell_pgid = getpid ();
        if (setpgid (shell_pgid, shell_pgid) < 0)
        {
            perror ("Couldn't put the shell in its own process group");
            exit (1);
        }

        // Grab control of the terminal.
        tcsetpgrp (shell_terminal, shell_pgid);

        // Save default terminal attributes for shell.
        tcgetattr (shell_terminal, &shell_tmodes);
    }
}

void delete_job(int p){
    int i;
    for(i=1;i<=jobsn;i++){
        if(jobs[i].p == p)
            break;
    }
    for(;i<jobsn;i++)
        jobs[i] = jobs[i+1];
    jobsn--;
}

// Funtion to execute built in commands - cd, pwd and echo
int built_in(char **input){
    if(strcmp(input[0],"cd")==0){
        if(input[1] == NULL){
            fprintf(stderr,"type a filename after cd\n");
        }
        else {
            int e = chdir(input[1]);
            if(e!=0)
                perror("wrong input\n");
            return 1;
        }
    }
    else if(strcmp(input[0],"echo")==0){
        if(input[1] == NULL)
        {
            printf("\n");
        }
        else{
            int i=1;
            while(input[i]!=NULL){
                printf("%s ",input[i++]);

            }
            printf("\n");
            return 1;
        }
    }
    else if(strcmp(input[0], "pwd") == 0 && input[1] == NULL){
        printf("%s\n",cwd);
        return 1;
    }
    else if(strcmp(input[0], "pwd")==0){
        fprintf(stderr,"write only pwd\n");
        return 1;
    }
    else if(strcmp(input[0], "jobs")==0){
        if(jobsn <= 0)
            printf("No running Jobs currently\n");
        else
        {int i;
            for(i = 1;i<=jobsn;i++)
                printf("[%d] %s [%d]\n",i,jobs[i].name,jobs[i].p);
        }
        return 1;
    }
    else if(strcmp(input[0],"fg") == 0){
        if(input[1] == NULL){
            printf("job id not specified\n");
            return 1;
        }
        else{
            int no = atoi(input[1]);
            if(no <= jobsn){

                kill(jobs[no].p,SIGCONT);
                waitpid(jobs[no].p,NULL,WUNTRACED);
                    delete_job(jobs[no].p);
                //tcsetpgrp (shell_terminal, shell_pgid);

            }
            else
                printf("Job no does not exist\n");
            return 1;
        }
    }
    else if(strcmp(input[0],"kjob") == 0){
        if(input[1] == NULL){
            printf("job id not specified\n");
            return 1;
        }
        else if(input[2] == NULL)
        {
            //printf("NO 2nd argument\n");
        }
        else{
            int no = atoi(input[1]);

            if(no <= jobsn && atoi(input[2])>=0 ){
                kill(jobs[no].p,atoi(input[2]));
            }
            else
                printf("Job no does not exist\n");
            return 1;
        }
    }
    else if(strcmp(input[0],"overkill") == 0){
        if(jobsn>0){
            int no;
            for(no = 1;jobsn>0;)
                kill(jobs[no].p,9);
        }

        else{
            printf("No jobs to kill\n");
        }
        return 1;
    }
    return 0;
}

// using exit
int shell_exit(char *line){
    if(strcmp(line,"quit") == 0)
        return 1;
    else 
        return 0;
}

void handle_signal(int signo)
{   if(signo == SIGINT){
    printf("\n%s@%s:%s$ ",username,hostname,cwd);
    fflush(stdout);

                       }
if(signo == SIGCHLD){
    union wait wstat;
    pid_t pid;
    while(1){
        pid = wait3 (&wstat, WNOHANG, (struct rusage *)NULL);
        if(pid == 0)
            return;
        else if (pid == -1)
            return;
        else{
            delete_job(pid);
            printf("\nExited with PID [%d]\n return code %d\n",pid,wstat.w_retcode);
        }
    }
}
}

typedef void (*sighandler_t)(int);

// function to read a line from user
char *read_line(){

    char *input=malloc(sizeof(char)*1024);

    int c = getchar();
    if(c == '\n'){
        input[0]='\n';
        return input ;
    }
    if (c == EOF){
        printf("\n");
        _exit(0);
    }

    int i=0;
    while(c!=EOF && c != '\n'){
        input[i++]=c;
        c = getchar();

        if( c == EOF || c == '\n'){
            input[i]='\0';

        }
    }

    return input;
}
//splitting commands
char **split(char *input){
    char **s_string = malloc(100*sizeof(char*));
    char *p_string;

    p_string = strtok(input," \n\t");

    int i = 0;

    while(p_string!=NULL){
        s_string[i++] = p_string;
        p_string = strtok(NULL, " \n\t");
    }
    s_string[i] = NULL;

    return s_string;
}

//splitting for pipe
char **splitp(char *input){
    char **s_string = malloc(100*sizeof(char*));
    char *p_string;

    p_string = strtok(input,"|");

    int i = 0;

    while(p_string!=NULL){
        s_string[i++] = p_string;
        p_string = strtok(NULL, "|");
    }
    s_string[i] = NULL;

    return s_string;
}

//forking process
void process_shell(char *line){

    int c_pipe=0;
    int bla;
    for(bla=0;line[bla]!=0;bla++)
    {
        if(line[bla] == '|')
            c_pipe++;
    }
    if(c_pipe > 0){
        char **pip=splitp(line);
        int PIPEIN = 0;
        int PIPEOUT =0;
        int fdx[2];


        for(bla=0;pip[bla]!=NULL;bla++){
            char **out = split(pip[bla]);


            pipe(fdx);

            int i;
            int bg = 0;
            for(i=0;out[i]!=NULL;i++)
            {
                if(out[i][0] == '&')
                {
                    out[i] = NULL;
                    bg =1;
                    break;
                }
            }

            if( line[0] != '\n'){
                int ot = built_in(out);
                if( ot == 1)
                    return;
            }

            signal (SIGCHLD, handle_signal);

            pid=fork();

            if( bg == 1){

                jobs[++jobsn].p =pid;
                strcpy(jobs[jobsn].name, out[i-1]);

            }

            if(pid == 0){

                for(i = 0;out[i]!=NULL;i++){
                    if(out[i][0] == '>')
                    {
                        int fd = open(out[i+1],O_RDONLY | O_WRONLY | O_CREAT,S_IRWXU);
                        dup2(fd,STDOUT_FILENO);
                        close(fd);
                        out[i] = NULL;
                        break;

                    }
                }


                for(i = 0;out[i]!=NULL;i++){
                    if(out[i][0] == '<')
                    {
                        int fd = open(out[i+1],O_RDONLY);
                        dup2(fd,STDIN_FILENO);
                        close(fd);
                        out[i] = NULL;
                        break;
                    }
                }

                if(PIPEIN != 0){
                    dup2(PIPEIN,STDIN_FILENO);
                    close(PIPEIN);
                }

                if(pip[bla+1] != NULL){
                    dup2(fdx[1],STDOUT_FILENO);
                }

                if(execvp(out[0],out) == -1){
                    perror("error wrong input");
                    exit(0);
                }
                exit(EXIT_FAILURE);
            }
            else if(pid<0){
                perror("no child process");
            }
            else{

                if( bg == 0){
                    waitpid(pid,&status,WUNTRACED);
                }

                PIPEIN = fdx[0];
                close(fdx[1]);
            }
            free(out);
        }
    }

    else{

        char **out = split(line);
        int i;
        int bg = 0;
        for(i=0;out[i]!=NULL;i++)
        {
            if(out[i][0] == '&')
            {
                out[i] = NULL;
                bg =1;
                break;
            }
        }

        if( line[0] != '\n'){
            int ot = built_in(out);
            if( ot == 1)
                return;
        }

        signal (SIGCHLD, handle_signal);

        pid=fork();

        if( bg == 1){

            jobs[++jobsn].p =pid;
            strcpy(jobs[jobsn].name, out[i-1]);
            // jobs[jobsn].name = out[i-1];

        }

        if(pid == 0){

            for(i = 0;out[i]!=NULL;i++){
                if(out[i][0] == '>')
                {
                    int fd = open(out[i+1],O_RDONLY | O_WRONLY | O_CREAT,S_IRWXU);
                    dup2(fd,STDOUT_FILENO);
                    close(fd);
                    out[i] = NULL;
                    break;

                }
            }

            for(i = 0;out[i]!=NULL;i++){
                if(out[i][0] == '<')
                {
                    int fd = open(out[i+1],O_RDONLY);
                    dup2(fd,STDIN_FILENO);
                    close(fd);
                    out[i] = NULL;
                    break;
                }
            }

            if(execvp(out[0],out) == -1){
                perror("error wrong input");
                exit(0);
            }
            exit(EXIT_FAILURE);
        }
        else if(pid<0){
            perror("no child process");
        }
        else{

            if( bg == 0){
                waitpid(pid,&status,WUNTRACED);
            }
        }
        free(out);
    }
}
//replacing present path by ~
int add_tilde(char *cwd2,char *hom){
    int i=0;
    if(strlen(cwd2)<strlen(hom)-3)
        return 0;
    while(hom[i+3] == cwd2[i] && cwd2[i]!='\0'){
        i++;
    }
    if(i >= strlen(hom)-3 ){
        int j=0;
        user[j++]='~';
        for(;cwd2[i]!='\0';i++){
            user[j++]=cwd2[i];
        }
        user[j]='\0';
        return 1;
    }
    return 0;

}
//to start shell with present directory
void call_home(char *h){
    char **out = split(h);
    int ot=built_in(out);
}

int main(){
    getcwd(cwd, sizeof(cwd));
    char ho[100];
    ho[0]='c';
    ho[1]='d';
    ho[2]=' ';
    int a;
    for(a=3;cwd[a-3]!='\0';a++){
        ho[a]=cwd[a-3];
    }
    ho[a]='\0';
    char hom[100];
    strncpy(hom,ho,strlen(ho));
    call_home(ho);

    while(1){

        //FLAG =0;

        signal(SIGINT, SIG_IGN);
        signal(SIGINT, handle_signal);
        

        strncpy(ho,hom,strlen(hom));

        getlogin_r(username,sizeof(username));
        gethostname(hostname, sizeof(hostname));
        getcwd(cwd, sizeof(cwd));

        int j=add_tilde(cwd,hom);

        if(j>0)
            printf("%s@%s:%s$ ",username,hostname,user);
        else
            printf("%s@%s:%s$ ",username,hostname,cwd);

        char *line = read_line();
        int flag = 0;

        int ch;
        for(ch = 0;line[ch]!='\0';ch++)
            if(line[ch]!=' ' && line[ch]!='\t')
            {
                flag=1;
                break;
            }
        if(flag==0)
            continue;

        if(strcmp(line,"cd") == 0 || strcmp(line,"cd ~") == 0 || strcmp(line,"cd ") == 0){
            call_home(ho);
            continue;}

        if(shell_exit(line) || line == NULL)
            return 0;

        if( line[0] == '\n')
            continue;

        int i;
        int count = 0;

        for(i=0;i<strlen(line);i++){
            if(line[i]==';'){
                count++;
            }
        }

        //Running semi colon separated commands
        if(count>0){
            count++;
            char *x;
            char *h;
            x = strtok_r(line,";",&h);
            while(count--){
                if(strcmp(x,"cd") == 0 || strcmp(x,"cd ~") == 0){
                    call_home(ho);
                    strncpy(ho,hom,strlen(hom));
                    continue;}
                process_shell(x);
                x = strtok_r(NULL,";",&h);
            }
            continue;
        }
        process_shell(line);
        free(line);
    }
    return 0;
}
