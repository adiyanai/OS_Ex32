#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>

#define SIZE 150
#define ARGS_NUM 2
#define PRINT_ERROR_AND_EXIT fprintf(stderr, "Error in system call\n"); exit(-1);
#define CLOSE_FILE close(fd);

typedef struct Configuration configuration;
struct Configuration {
    char directory[SIZE];
    char input_file[SIZE];
    char output_file[SIZE];
};

void read_conf (char *file1, configuration *conf_info) {
    int fd, i, j, lines_num;
    char input_buffer [3 * SIZE];
    ssize_t read_input;
    i = 0;
    j = 0;
    lines_num = 1;

    // tests whether the file exist and whether the files can be accessed for reading
    if (access(file1, F_OK) == 0 && access(file1, R_OK) == 0) {
        // try to open file
        if ((fd = open(file1, O_RDONLY)) == -1) {
            PRINT_ERROR_AND_EXIT;
        }
    } else {
        PRINT_ERROR_AND_EXIT;
    }

    // read all the configuration info
    read_input = read(fd, input_buffer, sizeof(input_buffer));
    if (read_input == -1) {
        PRINT_ERROR_AND_EXIT;
    }
    CLOSE_FILE;

    // split the configuration info to lines
    while (lines_num < 4) {
        if (input_buffer[i] == '\n') {
            j = 0;
            lines_num++;
        } else {
            switch (lines_num) {
                case 1:
                    conf_info->directory[j] = input_buffer[i];
                    break;
                case 2:
                    conf_info->input_file[j] = input_buffer[i];
                    break;
                case 3:
                    conf_info->output_file[j] = input_buffer[i];
                    break;
                default:
                    PRINT_ERROR_AND_EXIT;
            }
            j++;
        }
        i++;
    }
}

void run_file(configuration *conf_info, char directory[SIZE]) {
    pid_t pid = fork();
    if (pid == -1) {
        PRINT_ERROR_AND_EXIT;
    } else if (pid == 0) {
        // create the path of the comp.out
        char path[SIZE] = {0};
        strncpy(path, directory, strlen(directory));
        strcat(path, "/comp.out");

        // create a path for an output file
        char output_file[SIZE] = {0};
        strncpy(output_file, directory, strlen(directory));
        strcat(output_file, "/comp.out");

        // open input file
        // tests whether the file exist and whether the files can be accessed for reading
        int input;
        if (access(conf_info->input_file, F_OK) == 0 && access(conf_info->input_file, R_OK) == 0) {
            // try to open file
            if ((input = open(conf_info->input_file, O_RDONLY)) == -1) {
                PRINT_ERROR_AND_EXIT;
            }
        } else {
            PRINT_ERROR_AND_EXIT;
        }

        // open output file
        int output = open(output_file, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
        if (output == -1) {
            PRINT_ERROR_AND_EXIT;
        }

        // IO redirection
        if(dup2(input, STDIN_FILENO) == -1) {
            close(input);
            close(output);
            exit(EXIT_FAILURE);
        }
        if(dup2(output, STDIN_FILENO) == -1) {
            close(output);
            exit(EXIT_FAILURE);
        }

        // close files
        if (close(input) == -1) {
            PRINT_ERROR_AND_EXIT;
        }
        if (close(output) == -1) {
            PRINT_ERROR_AND_EXIT;
        }

        char *arguments[2] = {path, NULL};
        execvp(arguments[0], arguments);
        PRINT_ERROR_AND_EXIT;
    } else {
        int status;
        // wait for the child process to end
        int waitRet = waitpid(pid, &status, 0);
        if (waitRet == -1) {
            PRINT_ERROR_AND_EXIT;
        }
    }
}

void compile_and_run_file (struct dirent* dir_struct, char directory[150], configuration *conf_info) {
    pid_t pid = fork();
    if (pid == -1) {
        PRINT_ERROR_AND_EXIT;
    } else if (pid == 0) {
        // create the path of the c file
        char c_file_path[SIZE] = {0};
        strncpy(c_file_path, directory, strlen(directory));
        strcat(c_file_path, "/");
        strcat(c_file_path, dir_struct->d_name);

        // create the path of the output location
        char output_location[SIZE] = {0};
        strncpy(output_location, directory, strlen(directory));
        strcat(output_location, "/");
        strcat(output_location, "/comp.out");

        // create the input to the execvp
        char *arguments[5] = {"gcc", "-o", output_location, c_file_path, NULL};
        // gcc command
        execvp(arguments[0], arguments);
        PRINT_ERROR_AND_EXIT;
    } else {
        int status;
        // wait for the child process to end
        int waitRet = waitpid(pid, &status, 0);
        if (waitRet == -1) {
            PRINT_ERROR_AND_EXIT;
        }

        // if gcc doesn't work
        if (WEXITSTATUS(status) != 0) {
            printf("Didn't success to compile the c file: %s\n", dir_struct->d_name);
            // else, run file
        } else {
            run_file(conf_info, directory);
        }
    }
}


void in_directory(configuration *conf_info, char directory[150]) {
    DIR* dir;
    char dir_buff[SIZE];
    memset(dir_buff,0,strlen(dir_buff));
    struct dirent* dir_struct;
    strncpy(dir_buff, directory, strlen(directory));

    dir = opendir(directory);
    if (dir == NULL) {
        PRINT_ERROR_AND_EXIT;
    }

    dir_struct = readdir(dir);
    while (dir_struct != NULL) {
        if (!strcmp(dir_struct->d_name, ".") || !strcmp(dir_struct->d_name, "..")) {
            dir_struct = readdir(dir);
            continue;

        } else if (dir_struct->d_type == DT_DIR) {
            // create the path of the  directory
            memset(dir_buff,0,strlen(dir_buff));
            strncpy(dir_buff, directory, strlen(directory));
            strcat(dir_buff, "/");
            strcat(dir_buff, dir_struct->d_name);
            printf("%s is a directory\n",dir_buff);
            // get into the directory
            in_directory(conf_info, dir_buff);
            // return to the previous directory path
            memset(dir_buff,0,strlen(dir_buff));
            strncpy(dir_buff, directory, strlen(directory));

        } else if (!strcmp(dir_struct->d_name + (strlen(dir_struct->d_name))-2, ".c")) {
            printf("%s is a c file\n",dir_struct->d_name);
            compile_and_run_file(dir_struct, directory, conf_info);
        }
        dir_struct = readdir(dir);
    }
}


int main(int argc, char *argv[]) {
    char *path;
    configuration *conf_info = calloc(1, sizeof(configuration));
    if (conf_info == NULL) {
        exit(-1);
    }

    if (argc != ARGS_NUM) {
        printf("unexpected number of arguments\n");
        return -1;
    }

    path = argv[1];
    read_conf(path, conf_info);
    printf("%s\n",conf_info->directory);
    printf("%s\n",conf_info->input_file);
    printf("%s\n",conf_info->output_file);
    in_directory(conf_info, conf_info->directory);


    return 0;
}