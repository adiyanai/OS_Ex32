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

typedef struct Configuration {
    char directory[SIZE];
    char input_file[SIZE];
    char output_file[SIZE];
}configuration;

typedef struct Student {
    char name[SIZE];
    int grade;
    char reason[SIZE];
}student;

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

void compare_output (configuration *conf_info, student *student_info,  char output_path[SIZE]) {
    pid_t pid = fork();
    if (pid == -1) {
        PRINT_ERROR_AND_EXIT;
    } else if (pid == 0) {
        char cwd[SIZE] = {0};

        // get the current path
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            // create the path of the comp.out
            strcat(cwd, "/comp.out‬‬");
        } else {
            PRINT_ERROR_AND_EXIT;
        }

        char *arguments[4] = {cwd, conf_info->output_file, output_path, NULL};
        execvp(arguments[0], arguments);
        printf("lol\n");
        PRINT_ERROR_AND_EXIT;
    } else {
        int status, return_val;
        // wait for the child process to end
        int waitRet = waitpid(pid, &status, 0);
        if (waitRet == -1) {
            PRINT_ERROR_AND_EXIT;
        }

        if (WEXITSTATUS(status) == 0) {
            return;
        } else {
            return_val = WEXITSTATUS(status);
        }

        // the output is different from the correct output
        if (return_val == 2) {
            student_info->grade = 60;
            strncpy(student_info->reason, "BAD_OUTPUT‬‬", strlen("BAD_OUTPUT‬‬"));
            // the output is similar to the correct output
        } else if (return_val == 3) {
            student_info->grade = 80;
            strncpy(student_info->reason, "SIMILAR_OUTPUT", strlen("SIMILAR_OUTPUT‬‬"));
            // the output is the correct output
        } else if (return_val == 1) {
            student_info->grade = 100;
            strncpy(student_info->reason, "GREAT_JOB‬‬", strlen("GREAT_JOB‬‬"));
        }
    }
}

void run_file (configuration *conf_info, student *student_info, char directory[SIZE]) {
    char output_file[SIZE] = {0};
    // create a path for an output file
    strncpy(output_file, directory, strlen(directory));
    strcat(output_file, "/output.txt");

    pid_t pid = fork();
    if (pid == -1) {
        PRINT_ERROR_AND_EXIT;
    } else if (pid == 0) {
        // create the path of the comp.out
        char path[SIZE] = {0};
        strncpy(path, directory, strlen(directory));
        strcat(path, "/comp.out");

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
        int output = open(output_file, O_RDWR | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
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

        char *arguments[3] = {path, conf_info->input_file, NULL};
        execvp(arguments[0], arguments);
        PRINT_ERROR_AND_EXIT;
    } else {
        int status, time;
        time = 0;
        // wait for the child process to end
        while (!waitpid(pid, &status, WNOHANG) && time < 5) {
            sleep(1);
            time++;
        }

        // if the exercise run more than 5 seconds
        if (time == 5) {
            student_info->grade = 40;
            strncpy(student_info->reason, "TIMEOUT", strlen("TIMEOUT"));
            // else, compare the output with the correct output
        } else {
            compare_output(conf_info, student_info, output_file);
        }
    }
}

void compile_and_run_file (struct dirent* dir_struct, char directory[SIZE], configuration *conf_info, student *student_info) {
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
            student_info->grade = 20;
            strncpy(student_info->reason, "COMPILATION_ERROR‬‬", strlen("COMPILATION_ERROR‬‬"));
            // else, run file
        } else {
            run_file(conf_info, student_info, directory);
        }
    }
}

int in_directory (configuration *conf_info, student *student_info, char directory[SIZE]) {
    DIR* dir;
    char dir_buff[SIZE];
    int c_file_exists = 0;
    struct dirent* dir_struct;
    memset(dir_buff,0,strlen(dir_buff));
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
            // get into the directory
            int exist = in_directory(conf_info, student_info, dir_buff);
            if (c_file_exists == 0) {
                c_file_exists = exist;
            }
            // return to the previous directory path
            memset(dir_buff,0,strlen(dir_buff));
            strncpy(dir_buff, directory, strlen(directory));

        } else if (!strcmp(dir_struct->d_name + (strlen(dir_struct->d_name))-2, ".c")) {
            c_file_exists = 1;
            printf("%s is a c file\n",dir_struct->d_name);
            compile_and_run_file(dir_struct, directory, conf_info, student_info);
        }
        dir_struct = readdir(dir);
    }
    return c_file_exists;
}

int main (int argc, char *argv[]) {
    char *conf_path;
    configuration conf_info;

    if (argc != ARGS_NUM) {
        printf("unexpected number of arguments\n");
        return -1;
    }

    conf_path = argv[1];
    read_conf(conf_path, &conf_info);

    DIR* dir;
    student student_info;
    struct dirent* dir_struct;
    dir = opendir(conf_info.directory);
    if (dir == NULL) {
        PRINT_ERROR_AND_EXIT;
    }

    dir_struct = readdir(dir);
    while (dir_struct != NULL) {
        if (!strcmp(dir_struct->d_name, ".") || !strcmp(dir_struct->d_name, "..")) {
            dir_struct = readdir(dir);
            continue;
            // if its a directory-student
        } else if (dir_struct->d_type == DT_DIR) {
            // initialize student info
            memset(student_info.name,0,sizeof(student_info.name));
            memset(student_info.reason,0, sizeof(student_info.reason));
            // name of the student
            strncpy(student_info.name, dir_struct->d_name, strlen(dir_struct->d_name));

            // create path for the student directory
            char path_buff[SIZE] = {0};
            strncpy(path_buff, conf_info.directory, strlen(conf_info.directory));
            strcat(path_buff, "/");
            strcat(path_buff, dir_struct->d_name);
            printf("directory: %s\n", path_buff);
            // goes over the student directory
            int exist_c_file = in_directory(&conf_info, &student_info, path_buff);
            // if the student doesn't has c file
            if (exist_c_file == 0) {
                student_info.grade = 0;
                strncpy(student_info.reason, "‫‪NO_C_FILE‬‬", strlen("‫‪NO_C_FILE‬‬"));
            }
            printf("student grade: %d, student reason: %s\n", student_info.grade, student_info.reason);
        } else {
            dir_struct = readdir(dir);
            continue;
        }
        dir_struct = readdir(dir);
    }
    return 0;
}