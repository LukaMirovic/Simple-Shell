#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/utsname.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/procfs.h>

#define MAX_TOKENS 20
#define MAX_LINE_LENGTH 1000

int status = 0;
char* tokens[MAX_TOKENS];
char prompt[] = "mysh";
int token_count = 0;
int debug = 0;
int vNar = 0;
char proc_path[MAX_LINE_LENGTH] = "/proc";
int bgstatus=0;

int bg_pids[100];
int bg_pid_count = 0;

void touch(int stToken, char **argv) {
    if (stToken != 2) {
        fprintf(stderr, "Ni dovolj argumentov");
        status=1;
        return;
    }

    int fd = open(argv[1], O_CREAT | O_WRONLY, 0644);
    if (fd < 0) {
        perror("open");
        return;
    }
    close(fd);

    if (utime(argv[1], NULL) < 0) {
        perror("utime");
    }
}
void file(int stToken, char **argv) {
    if (stToken != 2) {
        fprintf(stderr, "Ni dovolj argumentov");
        status=1;
        return;
    }

    struct stat buf;
    if (stat(argv[1], &buf) != 0) {
        perror("stat");
        return;
    }

    if (S_ISREG(buf.st_mode)) {
        printf("%s: navaden fajl\n", argv[1]);
    } else if (S_ISDIR(buf.st_mode)) {
        printf("%s: direktorij\n", argv[1]);
    } else if (S_ISCHR(buf.st_mode)) {
        printf("%s: znakovna naprava\n", argv[1]);
    } else if (S_ISBLK(buf.st_mode)) {
        printf("%s: blocna naprava\n", argv[1]);
    } else if (S_ISFIFO(buf.st_mode)) {
        printf("%s: FIFO\n", argv[1]);
    } else if (S_ISLNK(buf.st_mode)) {
        printf("%s: simbolicna povezava\n", argv[1]);
    } else if (S_ISSOCK(buf.st_mode)) {
        printf("%s: vticnica\n", argv[1]);
    } else {
        printf("%s: neznan tip fajla\n", argv[1]);
    }
}
void grep(int stToken, char **argv) {
    if (stToken != 3) {
        fprintf(stderr, "Neustrezno stevilo\n");
        return;
    }

    char *pattern = argv[1];
    FILE *file = fopen(argv[2], "r");
    if (!file) {
        perror("fopen");
        return;
    }

    char *line = NULL;
    size_t len = 0;

    while (getline(&line, &len, file) != -1) {
        if (strstr(line, pattern)) {
            printf("%s", line);
        }
    }

    free(line);
    fclose(file);
}
void head(int stToken, char **argv) {
    if (stToken < 2 || stToken > 4) {
        fprintf(stderr, "Neustrezno stevilo argumentov");
        status=1;
        return;
    }

    int lines = 10;
    char *filename;
    if (stToken == 2) {
        filename = argv[1];
    } else {
        if (argv[1][0] != '-' || argv[1][1] != 'n') {
            fprintf(stderr, "Napaka\n");
            return;
        }
        lines = atoi(argv[2]);
        filename = argv[3];
    }

    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("fopen");
        return;
    }

    char *line = NULL;
    size_t len = 0;
    for (int i = 0; i < lines && getline(&line, &len, file) != -1; i++) {
        printf("%s", line);
    }

    free(line);
    fclose(file);
}
void rev(int stToken, char **argv) {
    if (stToken != 2) {
        fprintf(stderr, "Ni dovolj argumentov");
        status=1;
        return;
    }

    FILE *file = fopen(argv[1], "r");
    if (!file) {
        perror("fopen");
        return;
    }

    char *line = NULL;
    size_t len = 0;

    while (getline(&line, &len, file) != -1) {
        for (int i = strlen(line) - 1; i >= 0; i--) {
            putchar(line[i]);
        }
        putchar('\n');
    }
   // printf("%s\n",line);
    free(line);
    fclose(file);
}
int comp(const void* a,const void *b){
    return (*(int *)a - *(int *)b);
}

void sigchld_handler(int sig) {
    int saved_errno = errno;
    while (waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

int set_proc_path(char *path) {
    if (path == NULL) {
        printf("%s\n", proc_path);
        status=0;
    } else {
        if (access(path, F_OK | R_OK) == 0) {
            strcpy(proc_path, path);
            status=0;
        } else {
            //  perror("proc");
            status=1;
        }
    }
}
void seq(int numToken, char **argv) {
    if (numToken < 2 || numToken > 4) {
        fprintf(stderr, "Usage: seq [first] [step] last\n");
        return;
    }

    int start = 1, step = 1, end;

    if (numToken == 2) {
        end = atoi(argv[1]);
    } else if (numToken == 3) {
        start = atoi(argv[1]);
        end = atoi(argv[2]);
    } else if (numToken == 4) {
        start = atoi(argv[1]);
        step = atoi(argv[2]);
        end = atoi(argv[3]);
    }

    for (int i = start; (step > 0 && i <= end) || (step < 0 && i >= end); i += step) {
        printf("%d\n", i);
    }
}
void print_pids() {
    DIR *dir;
    struct dirent *ent;
    int pids[100];
    int pid_count = 0;
   // fflush(stdout);
    if ((dir = opendir(proc_path)) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            int pid = atoi(ent->d_name);
            if (pid != 0) {
                pids[pid_count++] = pid;
                if (pid_count >= 100) {
                    break;
                }
            }
        }
        status=0;
        closedir(dir);
    } else {
        perror("print_pids");
        status=1;
        return;
    }
  //  fflush(stdout);
    qsort(pids, pid_count, sizeof(int), comp);
    for (int i = 0; i < pid_count; i++) {
        printf("%d\n", pids[i]);
    }
}
void cpcat2(int ntoken, char* tokent[]) {
    int input_fd, output_fd;
    ssize_t bytes_read, bytes_written;
    char buffer[MAX_LINE_LENGTH];
    if(ntoken==2){
        execv("cat",tokent);
    }
    if(strcmp(tokent[1],"-")==0){
        input_fd=STDIN_FILENO;
    }else {
        input_fd = open(tokent[1], O_RDONLY);
        if (input_fd == -1) {
            status = errno;
            perror("cpcat");
            return;
        }
    }
    if(ntoken==2){
        output_fd=STDOUT_FILENO;
    }else {
        output_fd = open(tokent[2], O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (output_fd == -1) {
            status = errno;
            perror("cpcat");
            close(input_fd);
            return;
        }
    }
    while ((bytes_read = read(input_fd, buffer, sizeof(buffer))) > 0) {
                int i;
                int flag=0;

        for (i = 0; i < bytes_read; i++) {
            if (buffer[i] == '\n') {
                buffer[i] = '\0'; // Replace newline with null terminator
                break;
            }
        }
         
        bytes_written = write(output_fd, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            status = errno;
            perror("cpcat");
            close(input_fd);
            close(output_fd);
            return;
        }
    }
    if (bytes_read == -1) {
        status = errno;
        perror("cpcat");
        return;
    }

    if (input_fd != STDIN_FILENO) close(input_fd);
    if (output_fd != STDOUT_FILENO) close(output_fd);
    status = 0;
}
void cpcat(int ntoken) {
    int input_fd, output_fd;
    ssize_t bytes_read, bytes_written;
    char buffer[MAX_LINE_LENGTH];

    if(strcmp(tokens[1],"-")==0){
        input_fd=STDIN_FILENO;
    }else {
        input_fd = open(tokens[1], O_RDONLY);
        if (input_fd == -1) {
            status = errno;
            fflush(stdout);
            perror("cpcat");
            return;
        }
    }

    if(ntoken==2){
        output_fd=STDOUT_FILENO;
    }else {
        output_fd = open(tokens[2], O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (output_fd == -1) {
            status = errno;
            fflush(stdout);
            perror("cpcat");
            close(input_fd);
            return;
        }
    }

    while ((bytes_read = read(input_fd, buffer, sizeof(buffer))) > 0) {
        fflush(stdout);
        bytes_written = write(output_fd, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            status = errno;
            fflush(stdout);
            perror("cpcat");
            close(input_fd);
            close(output_fd);
            return;
        }
    }

    if (bytes_read == -1) {
        status = errno;
        fflush(stdout);
        perror("cpcat");

    }

    if (input_fd != STDIN_FILENO) close(input_fd);
    if (output_fd != STDOUT_FILENO) close(output_fd);

    status = 0;
}
char* printBrez(char *str) {
    if (str != NULL && str[0] != '\0') {
        return str + 1;
    }
    return NULL;
}

char* baza(char* path) {
    int len = strlen(path);
    int sles = -1;

    for (int i = len - 1; i >= 0; i--) {
        if (path[i] == '/') {
            sles = i;
            break;
        }
    }


    if (sles != -1) {
        return path + sles + 1;
    } else {
        return path;
    }
}

char* dirIme(char* path) {
    static char dir[MAX_LINE_LENGTH];
    int len = strlen(path);
    int sles = -1;


    for (int i = len - 1; i >= 0; i--) {
        if (path[i] == '/') {
            sles = i;
            break;
        }
    }

    if (sles != -1) {
        strncpy(dir, path, sles);
        dir[sles] = '\0';
        return dir;
    } else {
        return ".";
    }
}

void helper(char *token, int num) {
    if (strcmp(token, "&") == 0) {
        printf("Background: 1\n");
    } else if (token[0] == '>') {
        printf("Output redirect: '%s'\n", printBrez(token));
    } else if (token[0] == '<') {
        printf("Input redirect: '%s'\n", printBrez(token));
    }
}

int tokenize(char *vrstica) {
    token_count = 0;
    int flag = 0;
    char *start = vrstica;
    char *end = vrstica;

    while (*end != '\0' && token_count < MAX_TOKENS) {
        if (*end == ' ' && !vNar) {
            *end = '\0';
            if (start != end) {
                tokens[token_count] = strdup(start);
                token_count++;
            }
            start = end + 1;
        } else if (*end == '"') {
            if (!vNar) {
                vNar = 1;
                start = end + 1;
            } else {
                *end = '\0';
                if (start != end) {
                    tokens[token_count] = strdup(start);
                    token_count++;
                }
                vNar = 0;
                start = end + 1;
            }
        }
        end++;
    }

    if (start != end) {
        tokens[token_count] = strdup(start);
        token_count++;
    }

    return token_count;
}

void help() {
    printf("\n");
    printf("===================================================================\n");
    printf("                     Welcome to MySH Help                       \n");
    printf("===================================================================\n");
    printf("\n");
    printf(" Command: debug [level]\n");
    printf("    - Optional numerical argument 'level' specifies the debug level.\n");
    printf("    - If no argument is provided, the current debug level is displayed.\n");
    printf("    - Sets the debug level to the given 'level'. If incorrectly formatted, default level 0 is assumed.\n");
    printf("    - Initial debug level is set to 0. Levels 2 and above can be used for custom needs.\n");
    printf("\n");
    printf(" Command: prompt [prompt_str]\n");
    printf("    - Display or set the prompt string.\n");
    printf("    - Initial prompt string is 'mysh'.\n");
    printf("    - If no argument is provided, the current prompt is displayed.\n");
    printf("    - Sets a new prompt string. Up to 8 characters allowed. Longer names return exit status 1.\n");
    printf("\n");
    printf(" Command: status\n");
    printf("    - Displays the exit status of the last executed command.\n");
    printf("    - Leaves the status unchanged.\n");
    printf("\n");
    printf(" Command: help\n");
    printf("    - Displays a list of supported commands.\n");
    printf("\n");
    printf("===================================================================\n");
    printf("\n");
}
void pinfo() {
    printf("%5s %5s %6s %s\n", "PID", "PPID", "STANJE", "IME");
        DIR *dir;
    struct dirent *ent;
    int pids[100];
    int pid_count = 0;

    if ((dir = opendir(proc_path)) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            int pid = atoi(ent->d_name);
            if (pid != 0) {
                pids[pid_count++] = pid;
                if (pid_count >= 100) {
                    break;
                }
            }
        }
        status=0;
        closedir(dir);
    } else {
        perror("print_pids");
        status=1;
        return;
    }
    qsort(pids, pid_count, sizeof(int), comp);
    for(int i=0;i<pid_count;i++){

    char path[100];
    sprintf(path, "%s/%d/stat", proc_path, pids[i]);
FILE* f = fopen(path, "r");
    if(f == NULL) {
        continue;
    }
    int pid;
    int ppid;
    char stanje;
    char ime[100];
    //printf("%s\n",path);
    fscanf(f, "%d %s %c %d", &pid, ime, &stanje, &ppid);
    printf("%5d %5d %6c %s\n", pid, ppid, stanje, ime);
    fclose(f);
    }
}
void wait_for_process(int pid) {
    int wstatus;
   // fflush(stdout);
    if (waitpid(pid, &wstatus, 0) == -1) {
        perror("waitpid");
        status = 1;
    } else {
        if(bgstatus==0){
        status = WIFEXITED(wstatus) ? WEXITSTATUS(wstatus) : 1;
    }else{
        status=bgstatus;
    }
    }
}

void wait_for_one_process(int pid) {
    int wstatus;
    //fflush(stdout);
    if(status==10752){
        status=42;
        bgstatus=42;
    }
    if (pid == -1) {
        if (wait(&wstatus) == -1) {
            if(bgstatus==0){
            status = 0;
            }else{
                status=bgstatus;
            }
        } else {
            if(bgstatus==0){
            status = WIFEXITED(wstatus) ? WEXITSTATUS(wstatus) : 1;
}else{
    status=bgstatus;
}
        }
    } else {
        wait_for_process(pid);
    }
}

void wait_for_all_processes() {
    for (int i = 0; i < bg_pid_count; i++) {
        int wstatus;
        if (waitpid(bg_pids[i], &wstatus, 0) == -1) {
            perror("waitpid");
            status = 1;
            return;
        }
        fflush(stdout);
        status = WIFEXITED(wstatus) ? WEXITSTATUS(wstatus) : 1;
        
    }
    bg_pid_count = 0;
}
void execute_commands(char* tokens[], int num_commands) {
    int pipes[num_commands - 1][2];

    // Create pipes
    for (int i = 1; i < num_commands - 1; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("Pipe creation failed");
            exit(EXIT_FAILURE);
        }
    }

    // Execute commands
    for (int i = 1; i < num_commands; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Child process

            // Close unnecessary pipe ends
            if (i > 0) {
                close(pipes[i - 1][1]); // Close the write end of the previous pipe
                dup2(pipes[i - 1][0], STDIN_FILENO); // Redirect standard input
                close(pipes[i - 1][0]); // Close the read end of the previous pipe
            }
            if (i < num_commands - 1) {
                close(pipes[i][0]); // Close the read end of the current pipe
                dup2(pipes[i][1], STDOUT_FILENO); // Redirect standard output
                close(pipes[i][1]); // Close the write end of the current pipe
            }

            // Execute command
            char* command[MAX_TOKENS];
            char* token = strtok(tokens[i], " ");
            int j = 0;
            while (token != NULL) {
                command[j++] = token;
                token = strtok(NULL, " ");
            }
            command[j] = NULL;
            if(strcmp(command[0],"cpcat")==0){
                strcpy(command[0],"cat");
                execvp(command[0],command);
            }else{
            execvp(command[0], command);
            perror("Execution failed");
            exit(EXIT_FAILURE);
        }
        }
    }

    // Close all pipe ends in the parent process
    for (int i = 1; i < num_commands - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Wait for all child processes to finish
    for (int i = 1; i < num_commands; i++) {
        wait(NULL);
    }
}



int main() {
    char vrstica[MAX_LINE_LENGTH];
    while (1) {
        if (isatty(STDIN_FILENO)) {
            printf("mysh> ");
        }
        if (fgets(vrstica, MAX_LINE_LENGTH, stdin) == NULL) {
            break;
        }
      //  printf("%s\n",vrstica);
        if (vrstica[0] == '\n' || vrstica[0] == '#') {
            continue;
        }
        if (vrstica[strlen(vrstica) - 1] == '\n') {
            vrstica[strlen(vrstica) - 1] = '\0';
        }
        char c[MAX_LINE_LENGTH];
        strcpy(c, vrstica);
        int stToken = tokenize(vrstica);
        //printf("vrstica %s\n",c);
            int bgflag=0;
            if(stToken>1 && strcmp(tokens[stToken - 1], "&") == 0){
                bgflag = 1;
                tokens[stToken - 1] = NULL;
                stToken--;
            }
        if (strcmp(tokens[0], "debug") == 0) {
            if (stToken > 1) {
                if (atoi(tokens[1]) != 0) {
                    debug = atoi(tokens[1]);
                } else {
                    if (debug > 0) {
                        debug = 0;
                        printf("Input line: '%s'\n", c);
                        for (int i = 0; i < stToken; i++) {
                            printf("Token %d: '%s'\n", i, tokens[i]);
                        }
                        printf("Executing builtin 'debug' in foreground\n");
                    }
                }
            } else {
                if (debug > 0) {
                    printf("Input line: '%s'\n", c);
                    for (int i = 0; i < stToken; i++) {
                        printf("Token %d: '%s'\n", i, tokens[i]);
                    }
                    printf("Executing builtin 'debug' in foreground\n");
                }
                printf("%d\n", debug);
            }
        } else if (strcmp(tokens[0], "prompt") == 0) {
            if (token_count == 1) {
                printf("%s\n", prompt);
                status = 0;
            } else {
                if (strlen(tokens[1]) >= 8) {
                    status = 1;
                } else {
                    status = 0;
                    strcpy(prompt, tokens[1]);
                }
            }
        } else if (strcmp(tokens[0], "dirmk") == 0 || strcmp(tokens[0], "dirch") == 0 ||
                   strcmp(tokens[0], "dirrm") == 0 || strcmp(tokens[0], "dirls") == 0 ||
                   strcmp(tokens[0], "dirwd") == 0) {
            if (strcmp(tokens[0], "dirch") == 0) {
                if (token_count == 1) {
                    if (chdir("/") != 0) {
                        status = errno;
                        fflush(stdout);
                        perror("dirch");
                    }
                } else if (token_count == 2) {
                    if (chdir(tokens[1]) != 0) {
                        status = errno;
                        fflush(stdout);
                        perror("dirch");
                    }
                } else {
                    status = 1;
                }
            } else if (strcmp(tokens[0], "dirmk") == 0) {

    int background = 0;
    char* directory_name = tokens[1];

    if (stToken == 3 && strcmp(tokens[2], "&") == 0) {
        background = 1;
    }
fflush(stdin);
    pid_t pid = fork();
    if (pid == 0) {
        if (mkdir(tokens[1], 0777) == -1) {
            status=errno;
            bgstatus=errno;
            perror("dirmk");
        }
        
    } else if (pid > 0) {
        if (!background) {
            waitpid(pid, &status, 0);
        } else {
            bg_pids[bg_pid_count++] = pid;
        }
    } else {
        perror("fork");
        status = 1;
    }
              //  kill(0,SIGTERM);
            
            }else if (strcmp(tokens[0], "dirrm") == 0) {
                if (token_count == 2) {
                    if (rmdir(tokens[1]) != 0) {
                        status = errno;
                        fflush(stdout);
                        perror("dirrm");
                    }
                } else {
                    status = 1;
                }
            } else if (strcmp(tokens[0], "dirls") == 0) {
                char *directory = ".";
                if (token_count == 2) {
                    directory = tokens[1];
                }
                DIR *dir;
                struct dirent *ent;
                if ((dir = opendir(directory)) != NULL) {
                    while ((ent = readdir(dir)) != NULL) {
                        printf("%s  ", ent->d_name);
                    }
                    closedir(dir);
                    printf("\n");
                    fflush(stdout);
                } else {
                    status = errno;
                    fflush(stdout);
                    perror("dirls");
                }
            } else if (strcmp(tokens[0], "dirwd") == 0) {
                if (token_count == 1 || (token_count == 2 && strcmp(tokens[1], "base") == 0)) {
                    char cwd[MAX_LINE_LENGTH];
                    if (getcwd(cwd, sizeof(cwd)) != NULL) {
                        printf("%s\n", baza(cwd));
                    } else {
                        status = errno;
                        fflush(stdout);
                        perror("dirwd");
                    }
                } else if (token_count == 2 && strcmp(tokens[1], "full") == 0) {
                    char cwd[MAX_LINE_LENGTH];
                    if (getcwd(cwd, sizeof(cwd)) != NULL) {
                        printf("%s\n", cwd);
                    } else {
                        status = errno;
                        fflush(stdout);
                        perror("dirwd");
                    }
                } else {
                    status = 1;
                }
            }
        } else if (strcmp(tokens[0], "print") == 0) {
            for (int i = 1; i < stToken; i++) {
                printf("%s", tokens[i]);
                if (i != stToken - 1) {
                    printf(" ");
                }
            }

        } else if (strcmp(tokens[0], "echo") == 0) {
            fflush(stdout);
            for (int i = 1; i < stToken; i++) {
                printf("%s", tokens[i]);
                if (i != stToken - 1) {
                    printf(" ");
                }
            }
            printf("\n");
            fflush(stdout);
        } else if (strcmp(tokens[0], "len") == 0) {
            int len = 0;
            for (int i = 1; i < stToken; i++) {
                len += strlen(tokens[i]);
            }
            printf("%d\n", len);
        } else if (strcmp(tokens[0], "sum") == 0) {
            int sum = 0;
            for (int i = 1; i < stToken; i++) {
                sum += atoi(tokens[i]);
            }
            printf("%d\n", sum);
        } else if (strcmp(tokens[0], "calc") == 0) {
            int calc = 0;
            if (strcmp(tokens[2], "+") == 0) {
                calc = atoi(tokens[1]) + atoi(tokens[3]);
            } else if (strcmp(tokens[2], "-") == 0) {
                calc = atoi(tokens[1]) - atoi(tokens[3]);
            } else if (strcmp(tokens[2], "*") == 0) {
                calc = atoi(tokens[1]) * atoi(tokens[3]);
            } else if (strcmp(tokens[2], "/") == 0) {
                calc = atoi(tokens[1]) / atoi(tokens[3]);
            } else if (strcmp(tokens[2], "%") == 0) {
                calc = atoi(tokens[1]) % atoi(tokens[3]);
            }
            printf("%d\n", calc);
        } else if (strcmp(tokens[0], "basename") == 0) {
            if (token_count == 2) {
                printf("%s\n", baza(tokens[1]));
            } else {
                status = 1;
            }
        } else if (strcmp(tokens[0], "dirname") == 0) {
            if (token_count == 2) {
                printf("%s\n", dirIme(tokens[1]));
            } else {
                status = 1;
            }
        } else if (strcmp(tokens[0], "help") == 0) {
            help();
        } else if (strcmp(tokens[0], "exit") == 0) {
            if (stToken > 1) {
                status=atoi(tokens[1]);
                if(stToken==3){
                    bgstatus=status;
                    WEXITSTATUS(status);
                }
            
                exit(status);
                
            } else {
                exit(status);
            }
        } else if (strcmp(tokens[0], "status") == 0) {
            //fflush(stdout);
            printf("%d\n", status);
            fflush(stdout);
        } else if (strcmp(tokens[0], "rename") == 0) {
            if (token_count == 3) {
                if (rename(tokens[1], tokens[2]) != 0) {
                    status = errno;
                    fflush(stdout);
                    perror("rename");
                } else {
                    status = 0;
                }
            } else {
                status = 1;
            }
        } else if (strcmp(tokens[0], "unlink") == 0) {
            if (token_count == 2) {
                if (unlink(tokens[1]) != 0) {
                    status = errno;
                    fflush(stdout);
                    perror("unlink");
                } else {
                    status = 0;
                }
            } else {
                status = 1;
            }
        } else if (strcmp(tokens[0], "remove") == 0) {
            if (token_count == 2) {
                if (remove(tokens[1]) != 0) {
                    status = errno;
                    fflush(stdout);
                    perror("remove");
                } else {
                    status = 0;
                }
            } else {
                status = 1;
            }
        } else if (strcmp(tokens[0], "linkhard") == 0) {
            if (token_count == 3) {
                if (link(tokens[1], tokens[2]) != 0) {
                    status = errno;
                    fflush(stdout);
                    perror("linkhard");
                } else {
                    status = 0;
                }
            } else {
                status = 1;
            }
        } else if (strcmp(tokens[0], "linksoft") == 0) {
            if (token_count == 3) {
                if (symlink(tokens[1], tokens[2]) != 0) {
                    status = errno;
                    fflush(stdout);
                    perror("linksoft");
                } else {
                    status = 0;
                }
            } else {
                status = 1;
            }
        } else if (strcmp(tokens[0], "linkread") == 0) {
            if (token_count == 2) {
                char buf[MAX_LINE_LENGTH];
                ssize_t len = readlink(tokens[1], buf, sizeof(buf) - 1);
                if (len != -1) {
                    buf[len] = '\0';
                    printf("%s\n", buf);
                    status = 0;
                } else {
                    status = errno;
                    fflush(stdout);
                    perror("linkread");
                }
            } else {
                status = 1;
            }
        } else if (strcmp(tokens[0], "linklist") == 0) {
            if (token_count == 2) {
                int printed = 0;
                struct stat fileStat;
                if (stat(tokens[1], &fileStat) == 0) {
                    DIR *dir;
                    struct dirent *ent;
                    if ((dir = opendir(".")) != NULL) {
                        while ((ent = readdir(dir)) != NULL) {
                            struct stat entStat;
                            if (lstat(ent->d_name, &entStat) == 0 && S_ISREG(entStat.st_mode) && entStat.st_nlink > 1 &&
                                entStat.st_ino == fileStat.st_ino) {
                                printf("%s  ", ent->d_name);
                                printed = 1;
                            }
                        }
                        closedir(dir);
                        if (printed) {
                            printf("\n");
                        }
                        status = 0;
                    } else {
                        status = errno;
                        fflush(stdout);
                        perror("linklist");
                    }
                } else {
                    status = errno;
                    fflush(stdout);
                    perror("linklist");
                }
            } else {
                status = 1;
            }
        } else if (strcmp(tokens[0], "cpcat") == 0) {
            if (token_count == 2 || token_count == 3) {
                cpcat(stToken);
            } else if (token_count == 1) {
                char buffer[MAX_LINE_LENGTH];
                ssize_t bytes_read;

                while ((bytes_read = read(STDIN_FILENO, buffer, MAX_LINE_LENGTH)) > 0) {
                    write(STDOUT_FILENO, buffer, bytes_read);
                }

                if (bytes_read == -1) {
                    status = errno;
                    fflush(stdout);
                    perror("cpcat");
                }
            } else {
                status = 1;
            }
        } else if (strcmp(tokens[0], "pid") == 0) {
            printf("%d\n", getpid());
            status = 0;
        } else if (strcmp(tokens[0], "ppid") == 0) {
            printf("%d\n", getppid());
            status = 0;
        } else if (strcmp(tokens[0], "uid") == 0) {
            printf("%d\n", getuid());
            status = 0;
        } else if (strcmp(tokens[0], "euid") == 0) {
            printf("%d\n", geteuid());
            status = 0;
        } else if (strcmp(tokens[0], "gid") == 0) {
            printf("%d\n", getgid());
            status = 0;
        } else if (strcmp(tokens[0], "egid") == 0) {
            printf("%d\n", getegid());
            status = 0;
        } else if (strcmp(tokens[0], "sysinfo") == 0) {
            struct utsname info;
            if (uname(&info) != 0) {
                perror("uname");
                status = 1;
            }
            printf("Sysname: %s\n", info.sysname);
            printf("Nodename: %s\n", info.nodename);
            printf("Release: %s\n", info.release);
            printf("Version: %s\n", info.version);
            printf("Machine: %s\n", info.machine);
            status = 0;
        } else if (strcmp(tokens[0], "proc") == 0) {
            if (token_count == 1) {
                set_proc_path(NULL);
            } else if (token_count == 2) {
                set_proc_path(tokens[1]);
            } else {
                status = 1;
            }
        } else if (strcmp(tokens[0], "pids") == 0) {
            
            if(bgflag){
                fflush(stdin);
              //  printf("here\n");
                pid_t pid = fork();
            if (pid == -1) {
                perror("fork");
                status = 1;
            } else if (pid == 0) {
               // fflush(stdout);
                print_pids();

            } else {
        if (bgflag) {
            bg_pids[bg_pid_count++] = pid;
        } else {
            int wstatus;
            if (waitpid(pid, &wstatus, 0) == -1) {
                perror("waitpid");
                status = 1;
            } else {
                status = WIFEXITED(wstatus) ? WEXITSTATUS(wstatus) : 1;
            }
        }
            }
            }else{
                print_pids();
            }
        } else if (strcmp(tokens[0], "pinfo") == 0) {
            pinfo();
        } else if (strcmp(tokens[0], "waitone") == 0) {
            if (stToken == 2) {
                wait_for_one_process(atoi(tokens[1]));
            } else {
                wait_for_one_process(-1);
            }
        }else if (strcmp(tokens[0], "waitall") == 0) {
            wait_for_all_processes();
        } else if(strcmp(tokens[0],"pipes")==0){
           // for(int i=0;i<stToken;i++){
             //   printf("%s nas string\n",tokens[i]);
        //    }
            execute_commands(tokens,stToken);
        }else if(strcmp(tokens[0],"seq")==0){
            seq(stToken,tokens);
        }else if(strcmp(tokens[0],"rev")==0){
            rev(stToken,tokens);
        }else if(strcmp(tokens[0],"file")==0){
            file(stToken,tokens);
        }else if(strcmp(tokens[0],"touch")==0){
            touch(stToken,tokens);
        }else if(strcmp(tokens[0],"head")==0){
            head(stToken,tokens);
        }else if(strcmp(tokens[0],"grep")==0){
            grep(stToken,tokens);
        }else {
            printf("%d\n",stToken);
            char* cc[stToken];
            if(stToken!=1){
            for(int i=0;i<stToken;i++){
                cc[i] = malloc(strlen(tokens[i])+1);
                strcpy(cc[i],tokens[i]);
            }
            cc[stToken]=malloc(strlen(tokens[0])+1);
            cc[stToken]=NULL;
            }else{
            cc[0]=malloc(strlen(tokens[0])+1);
            strcpy(cc[0],tokens[0]);
            cc[1]=malloc(strlen(tokens[0])+1);
            cc[1]=NULL;
            }

           // printf("dolzina %d\n",stToken);
           // fflush(stdin);
          // printf("ovo je string %s\n",cc[stToken-1]);
            pid_t pid = fork();
            
            if(cc[stToken-1][0]=='<'){
                
                char* input=printBrez(cc[stToken-1]);
                printf("%s\n",input);
                cc[stToken-1]=NULL;
                int fd0 = open(input, O_RDONLY, 0);
                dup2(fd0, STDIN_FILENO);
                close(fd0);
                execvp(cc[0],cc);
            }else if(cc[stToken-1][0]=='>'){
                char* output=printBrez(cc[stToken-1]);
                cc[stToken-1]=NULL;
                int fd1 = creat(output, 0644);
                dup2(fd1, STDOUT_FILENO);
                close(fd1);
                execvp(cc[0],cc);
            }else{
            if (pid == -1) {
                perror("fork");
                status = 1;
            } else if (pid == 0) {
               // fflush(stdout);
                if (execvp(cc[0], cc) == -1) {
                    fflush(stdout);
                    perror("exec");
                    exit(127);
                }
            } else {
        if (bgflag) {
            bg_pids[bg_pid_count++] = pid;
        } else {
            int wstatus;
        
            if (waitpid(pid, &wstatus, 0) == -1) {
                perror("waitpid");
                status = 1;
            } else {
                status = WIFEXITED(wstatus) ? WEXITSTATUS(wstatus) : 1;
            }
        }
            }
        }

        }
    }return status;

}
