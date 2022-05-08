#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/wait.h>

bool BG = false;
int cmdCount = 0, TotalWord = 0;

//return how many lines in the history file
int countLine();

//return how many '|' in the string and the two index of them
int countPipe(char *, int *, int *);

//return how many '!' in the string and the two index of them
int exclMark(char *, int *, int *);

//convert the number of line from the history to string
void convertToString(int, char []);

//check if the number entered after the '!' is correct.
int checkHistory(char [], int, int);

//split the string to array word by word.
void splitToArray(char *[], char[]);

//execute 2 commands with 1 pipe
void pipeTwoCmd(char *, int);

//execute 3 commands with 2 pipes
void pipeThreeCmd(char *, int, int);

//count how many word and character in the string and return the first word in the string
const char *count(char[], int *, int *);

//print the history with the line number
void history();

void loop();

//write to the file any string you enter
void writeToFile(char *);

void ex(int, char *, int *, int *);

//free the alloc for 3 arr
void freeArr3(char *[], char *[], char *[], int, int, int);

//free the alloc for 2 arr
void freeArr2(char *[], char *[], int, int);

//to let the background process to work in it, and let the father continue, after the child finished his job the father free him to prevent him form enter zombie
void handler(int sig) {
    int waitN = 1;
    while (waitN != -1 && waitN != 0) {
        waitN = waitpid(-1, &sig, WNOHANG);
    }
}

int main() {
    signal(SIGCHLD, handler);
    loop();
}


void loop() {
    //I added 2 to the length because there's '\n\0' at the end of the str
    char str[514];
    char cwd[PATH_MAX];

    while (-1) {
        int ind1 = -1, ind2 = -1, pipeNum = 0;

        int charCount = 0, wordCount = 0;
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s>", cwd);
            fgets(str, 514, stdin);
            //because the user press enter so '\n' enter to the input string in the last index, so we put '\0'
            str[strlen(str) - 1] = '\0';
            int fMark = -1, sMark = -1;
            //count how many exclamation mark and pipe in the string
            int countExcl = exclMark(str, &fMark, &sMark);
            pipeNum = countPipe(str, &ind1, &ind2);
            //if the command is background
            if (str[strlen(str) - 1] == '&')
                BG = true;
            else
                BG = false;

            //check if there's space after or before the command
            if ((str[0] == ' ' || str[strlen(str) - 1] == ' ') && !BG) {
                fprintf(stderr, "You have entered space/s before/after the command \n");
                continue;
            }
            //this section to execute the commands from the history
            if (countExcl == 1) {
                if (str[fMark + 1] == ' ')
                    fprintf(stderr, "You have entered space/s after the '!' \n");
                if (fMark < ind1) {//if the '!' before the | so the first command from history and the second from the user
                    char number[ind1];
                    /*
                     * copy to number the string after the '!' and before the '|' and convert it to number after that convert the number to the string according to the line in the history
                     */
                    strncpy(number, &str[1], ind1);
                    int line = atoi(number);
                    int numberOfLines = countLine();
                    //check if the line that the user enter is exist in the history file
                    if (line > numberOfLines || line < 1) {
                        printf("the line number doesn't exist\n");
                        continue;
                    }
                    char cmd1[512], cmd2[512];
                    convertToString(line, cmd1);
                    char FinalCmd1[strlen(cmd1) + 1];

                    //copy the converted string to the user command to one string and count again how many pipe and execute the command according to that
                    strcpy(FinalCmd1, cmd1);
                    strcpy(cmd2, &str[ind1]);
                    strcat(FinalCmd1, cmd2);

                    printf("%s\n", FinalCmd1);
                    if(FinalCmd1[strlen(FinalCmd1)-1]=='&')
                        BG=true;
                    pipeNum = countPipe(FinalCmd1, &ind1, &ind2);
                    if (pipeNum == 1)
                        pipeTwoCmd(FinalCmd1, ind1);
                    else if (pipeNum == 2)
                        pipeThreeCmd(FinalCmd1, ind1, ind2);
                    else
                        fprintf(stderr, "Currently, we can't execute more than 2 pipes \n");

                } else {
                    //same like the above if, but this condition is the history command after the '|'
                    char number[strlen(str) - fMark];
                    strcpy(number, &str[fMark + 1]);
                    int line = atoi(number);
                    int numberOfLines = countLine();
                    //check if the line that the user enter is exist in the history file
                    if (line > numberOfLines || line < 1) {
                        printf("the line number doesn't exist\n");
                        continue;
                    }
                    char cmd1[512], cmd2[512];
                    convertToString(line, cmd1);
                    char FinalCmd1[strlen(cmd1) + 1];

                    strcpy(FinalCmd1, cmd1);
                    strncpy(cmd2, &str[0], ind1 + 1);
                    cmd2[ind1 + 1] = '\0';
                    strcat(cmd2, FinalCmd1);

                    printf("%s\n", cmd2);
                    if(cmd2[strlen(cmd2)-1]=='&')
                        BG=true;
                    pipeNum = countPipe(cmd2, &ind1, &ind2);
                    if (pipeNum == 0) {
                        wordCount = 0, charCount = 0;
                        const char *word = count(cmd2, &charCount, &wordCount);
                        if (strcmp(word, "history") == 0) {
                            history();
                            writeToFile("history");
                        } else {
                            ex(wordCount, cmd2, &TotalWord, &cmdCount);
                            cmdCount++;
                            TotalWord += wordCount;
                        }
                    } else if (pipeNum == 1) {
                        pipeTwoCmd(cmd2, ind1);
                    } else if (pipeNum == 2)
                        pipeThreeCmd(cmd2, ind1, ind2);
                    else
                        fprintf(stderr, "Currently, we can't execute more than 2 pipes \n");
                }
            } else if (countExcl == 2) {
                if (str[fMark + 1] == ' ' || str[sMark + 1] == ' ')
                    fprintf(stderr, "You have entered space/s after the '!' \n");
                int check = 0;
                if (pipeNum != 2) {
                    if (ind1 == -1) {
                        check = checkHistory(str, 0, (int) strlen(str));
                        if (check == 1)
                            printf("enter only numbers after '!'\n");
                        else {
                            int Line = atoi(&str[1]);
                            int numberOfLines = countLine();
                            //check if the line that the user enter is exist in the history file
                            if (Line > numberOfLines || Line < 1) {
                                printf("the line number doesn't exist\n");
                                continue;
                            }
                            char cmd[514];
                            convertToString(Line, cmd);
                            printf("%s",cmd);
                            if(cmd[strlen(cmd)-1]=='&')
                                BG=true;
                            const char *word = count(cmd, &wordCount, &charCount);
                            if (strcmp(word, "history") == 0) {
                                history();
                                writeToFile("history");
                            } else
                                ex(wordCount, cmd, &TotalWord, &cmdCount);
                        }
                    } else {
                        int check1 = checkHistory(str, 0, ind1);
                        int check2 = checkHistory(str, sMark, (int) strlen(str));
                        if (check1 == 1 || check2 == 1)
                            fprintf(stderr, "enter only numbers after '!'\n");
                        char number[ind1];
                        strncpy(number, &str[1], ind1);
                        int line = atoi(number);
                        int numberOfLines = countLine();
                        //check if the line that the user enter is exist in the history file
                        if (line > numberOfLines || line < 1) {
                            printf("the line number doesn't exist\n");
                            continue;
                        }
                        char cmd1[512], cmd2[512];
                        convertToString(line, cmd1);
                        char FinalCmd1[strlen(cmd1) + 2];
                        strcpy(FinalCmd1, cmd1);
                        FinalCmd1[strlen(FinalCmd1)] = '\0';
                        strcat(FinalCmd1, "|");

                        strcpy(number, &str[sMark + 1]);
                        line = atoi(number);
                        numberOfLines = countLine();
                        //check if the line that the user enter is exist in the history file
                        if (line > numberOfLines || line < 1) {
                            printf("the line number doesn't exist\n");
                            continue;
                        }
                        convertToString(line, cmd2);
                        strcat(FinalCmd1, cmd2);
                        printf("%s\n", FinalCmd1);
                        if(FinalCmd1[strlen(FinalCmd1)-1]=='&')
                            BG=true;
                        pipeNum = countPipe(FinalCmd1, &ind1, &ind2);
                        pipeTwoCmd(FinalCmd1, ind1);
                    }
                } else
                    fprintf(stderr, "can't work with more than one '|' and '!' together\n");
            } else {
                if(countExcl>2)
                    continue;
                if (pipeNum > 2)
                    fprintf(stderr, "Currently, we can't execute more than 2 pipes \n");
                if (pipeNum == 1)
                    pipeTwoCmd(str, ind1);

                if (pipeNum == 2)
                    pipeThreeCmd(str, ind1, ind2);

                if (pipeNum == 0) {
                    //count method return an "cd", "done" or ""
                    const char *word = count(str, &charCount, &wordCount);
                    if (wordCount != 0) {
                        //if it's history, so we don't need to fork and execvp because cd not supported yet
                        if (strcmp(word, "history") == 0) {
                            //writeToFile method to save the string to history file
                            history();
                            cmdCount++;
                            TotalWord++;
                            writeToFile(str);
                        }
                            //if it's cd, so we don't need to fork and execvp because cd not supported yet
                        else if (strcmp(word, "cd") == 0) {
                            printf("command not supported (Yet)\n");
                            cmdCount++;
                            TotalWord += wordCount;
                        } else if (wordCount == 1 && strcmp(word, "done") == 0) {
                            printf("Num of commands: %d\n", (cmdCount + 1));
                            printf("Total number of words in all commands: %d !\n", TotalWord);
                            break;
                        } // else if it "done" print the total of commands and the words at the commands and stop the shell
                        else
                            ex(wordCount, str, &TotalWord, &cmdCount);

                    }
                }
            }
        }
    }

}


void history() {
    //print file history to the screen character by character
    int lineCount = countLine();
    int count = 1;
    FILE *readFile;
    readFile = fopen("file.txt", "r");
    if (readFile == NULL) {
        perror("file doesn't exist");
        exit(EXIT_FAILURE);

    } else {
        char c = (char) fgetc(readFile);//read character by character and print it to the
        if (lineCount != 0)
            printf("%d: ", count);
        count++;
        while (c != EOF) {
            printf("%c", c);
            if (c == '\n' && count != lineCount + 1) {
                printf("%d: ", count);
                count++;
            }
            c = (char) fgetc(readFile);
        }
        fclose(readFile);
    }
}


int countLine() {
    //count how many lines in the file
    int counter = 0;
    FILE *readFile;
    readFile = fopen("file.txt", "r");
    if (readFile == NULL) {
        perror("file doesn't exist");
        exit(EXIT_FAILURE);
    } else {
        char c = (char) fgetc(readFile);
        while (c != EOF) {
            if (c == '\n') counter++;
            c = (char) fgetc(readFile);

        }
        fclose(readFile);
    }
    return counter;
}

void pipeThreeCmd(char *str, int ind1, int ind2) {
    writeToFile(str);
    int charCount = 0;
    //split the three command every one to string and array.
    char firstCmd[ind1 + 1];
    strncpy(firstCmd, str, ind1);
    firstCmd[ind1] = '\0';

    char secCmd[ind2 - ind1];
    strncpy(secCmd, &str[ind1 + 1], ind2 - ind1 - 1);

    char thirdCmd[strlen(str) - ind2];
    strcpy(thirdCmd, &str[ind2 + 1]);

    /*
     * count the words in every command and add it to the total word and make array with size of every word in every command
     * and save the first word of every one of him to different parameter
     */
    int WC1 = 0;
    const char *cmd1 = count(firstCmd, &charCount, &WC1);
    char *cmdArr1[WC1 + 1];
    splitToArray(cmdArr1, firstCmd);
    TotalWord += WC1;
    int WC2 = 0;
    charCount = 0;
    const char *cmd2 = count(secCmd, &charCount, &WC2);
    char *cmdArr2[WC2 + 1];
    splitToArray(cmdArr2, secCmd);
    TotalWord += WC2;
    int WC3 = 0;
    charCount = 0;
    const char *cmd3 = count(thirdCmd, &charCount, &WC3);
    char *cmdArr3[WC3 + 1];
    splitToArray(cmdArr3, thirdCmd);
    TotalWord += WC3;

    //define 2 pipe to open and 3 child to execute a 3 command
    int pipeLM[2];
    int pipeMR[2];
    int status;
    pid_t pidLeft, pidRight, pidMid;

    //open the 2 pipe
    if (pipe(pipeLM) < 0 || pipe(pipeMR) < 0) {
        perror("pipe failed");
        freeArr3(cmdArr1, cmdArr2, cmdArr3, WC1, WC2, WC3);
        exit(EXIT_FAILURE);
    }
    //execute the first command and switch the output file to the first write pipe file
    if ((pidLeft = fork()) < 0) {
        close(pipeMR[0]);
        close(pipeMR[1]);
        close(pipeLM[0]);
        close(pipeLM[1]);
        freeArr3(cmdArr1, cmdArr2, cmdArr3, WC1, WC2, WC3);
        perror("fork Failed");
        exit(EXIT_FAILURE);
    } else if (pidLeft == 0) {
        close(pipeLM[0]);
        close(pipeMR[0]);
        close(pipeMR[1]);
        dup2(pipeLM[1], STDOUT_FILENO);
        if (strcmp(cmd1, "history") == 0)
            history();
        else if (-1 == execvp(cmdArr1[0], cmdArr1)) {
            perror("command not supported (Yet)");
            exit(EXIT_FAILURE);
        }
        close(pipeLM[1]);
        exit(EXIT_SUCCESS);
    } else {
        //create a new child to execute the second command
        if ((pidMid = fork()) < 0) {
            perror("fork Failed");
            close(pipeMR[0]);
            close(pipeMR[1]);
            close(pipeLM[0]);
            close(pipeLM[1]);
            freeArr3(cmdArr1, cmdArr2, cmdArr3, WC1, WC2, WC3);
            exit(EXIT_FAILURE);
        } else if (pidMid == 0) {
            //switch the pipe read file to standard input file, then the output file to the second pipe write file
            close(pipeLM[1]);
            close(pipeMR[0]);
            dup2(pipeLM[0], STDIN_FILENO);
            dup2(pipeMR[1], STDOUT_FILENO);
            //if the command is history execute the history
            if (strcmp(cmd2, "history") == 0)
                history();

            else if (-1 == execvp(cmdArr2[0], cmdArr2)) {
                perror("command not supported (Yet)");
                exit(EXIT_FAILURE);
            }
            close(pipeLM[0]);
            close(pipeMR[1]);
            exit(EXIT_SUCCESS);
        } else {
            if ((pidRight = fork()) < 0) {
                perror("fork Failed");
                close(pipeMR[0]);
                close(pipeMR[1]);
                close(pipeLM[0]);
                close(pipeLM[1]);
                freeArr3(cmdArr1, cmdArr2, cmdArr3, WC1, WC2, WC3);
                exit(EXIT_FAILURE);
            } else if (pidRight == 0) {
                //execute the third command after create the third child.
                //switch the pipe read file to read from the standard input file.
                close(pipeLM[0]);
                close(pipeLM[1]);
                close(pipeMR[1]);
                dup2(pipeMR[0], STDIN_FILENO);
                if (strcmp(cmd3, "history") == 0)
                    history();
                if (-1 == execvp(cmdArr3[0], cmdArr3)) {
                    perror("command not supported (Yet)");
                    exit(EXIT_FAILURE);
                }
                close(pipeMR[0]);
                exit(EXIT_SUCCESS);
            } else {
                //close all the file that opened from the pipes and free allocated memory
                close(pipeLM[0]);
                close(pipeLM[1]);
                close(pipeMR[0]);
                close(pipeMR[1]);
                freeArr3(cmdArr1, cmdArr2, cmdArr3, WC1, WC2, WC3);
                //if the command not background one, so let the father wait for all sons to finish
                if (!BG) {
                    wait(&status);
                    wait(&status);
                    wait(&status);
                }
                cmdCount++;
                TotalWord += 2;//because there's 2 pipe so there's "|" to of this
            }
        }

    }
}

//the same of execute two pipe three command
void pipeTwoCmd(char *str, int ind1) {

    writeToFile(str);
    int WC1 = 0, WC2 = 0, charCount = 0;
    char firstCmd[ind1 + 1];
    strncpy(firstCmd, str, ind1);
    firstCmd[ind1] = '\0';
    const char *cmd1 = count(firstCmd, &charCount, &WC1);
    char *cmdArr1[WC1 + 1];
    splitToArray(cmdArr1, firstCmd);
    TotalWord += WC1;
    char secCmd[strlen(str) - ind1];
    strcpy(secCmd, &str[ind1 + 1]);


    charCount = 0;
    const char *cmd2 = count(secCmd, &charCount, &WC2);
    char *cmdArr2[WC2 + 1];
    splitToArray(cmdArr2, secCmd);
    TotalWord += WC2;

    int pipeFD[2];

    int status;
    pid_t pidLeft, pidRight;

    if (pipe(pipeFD) < 0) {
        perror("pipe failed");
        freeArr2(cmdArr1, cmdArr2, WC1, WC2);
        exit(EXIT_FAILURE);
    }
    if ((pidLeft = fork()) < 0) {
        perror("fork Failed");
        close(pipeFD[0]);
        close(pipeFD[1]);
        freeArr2(cmdArr1, cmdArr2, WC1, WC2);
        exit(EXIT_FAILURE);
    } else if (pidLeft == 0) {
        close(pipeFD[0]);
        dup2(pipeFD[1], STDOUT_FILENO);
        if (strcmp(cmd1, "history") == 0) {
            history();
        } else if (-1 == execvp(cmdArr1[0], cmdArr1)) {
            perror("command not supported (Yet)");
            exit(EXIT_FAILURE);
        }
        close(pipeFD[1]);
        exit(EXIT_SUCCESS);
    } else {
        if ((pidRight = fork()) < 0) {
            perror("fork Failed");
            close(pipeFD[0]);
            close(pipeFD[1]);
            freeArr2(cmdArr1, cmdArr2, WC1, WC2);
            exit(EXIT_FAILURE);
        } else if (pidRight == 0) {
            close(pipeFD[1]);
            dup2(pipeFD[0], STDIN_FILENO);
            if (strcmp(cmd2, "history") == 0) {
                history();
            } else if (-1 == execvp(cmdArr2[0], cmdArr2)) {
                perror("command not supported (Yet)");
                exit(EXIT_FAILURE);
            }
            close(pipeFD[0]);
            exit(EXIT_SUCCESS);
        } else {
            close(pipeFD[0]);
            close(pipeFD[1]);
            freeArr2(cmdArr1, cmdArr2, WC1, WC2);
            if (!BG) {
                wait(&status);
                wait(&status);
            }
            cmdCount++;
            TotalWord++;

        }
    }

}

int checkHistory(char str[], int start, int ind1) {
    int k = start + 1;
    int check = 0;
    //check is like boolean to check if there's no anything other numbers after '!'
    if (str[k] == ' ')
        return 1;
    if (ind1 != -1) {
        while (k < ind1) {
            if ((str[k] <= '0' || str[k] >= '9') && str[k] != ' ')
                check = 1;
            k++;
        }
    }
    return check;

}

void convertToString(int line, char str[]) {
    char cmd[514];
    FILE *read;
    read = fopen("file.txt", "r");
    if (read == NULL) {
        perror("can't open file");
        exit(EXIT_FAILURE);
    } else {


        int i = 0;
        //override line to reach the goal line
        while (i < line) {
            fgets(cmd, 514, read);
            i++;
        }
        cmd[strlen(cmd) - 1] = '\0';
    }
    fclose(read);
    strcpy(str, cmd);
}


int exclMark(char *str, int *ind1, int *ind2) {
    int len = strlen(str), i = 0;
    int count = 0;
    while (i < len) {
        if (str[i] == '!') {
            count++;
            if (count == 1) *ind1 = i;
            if (count == 2) *ind2 = i;
        }

        i++;
    }
    return count;
}

int countPipe(char *str, int *ind1, int *ind2) {
    int len = strlen(str), i = 0;
    int count = 0;
    while (i < len) {
        if (str[i] == '|') {
            count++;
            if (count == 1) *ind1 = i;
            if (count == 2) *ind2 = i;
        }

        i++;
    }
    return count;
}


//add the input string to the file.txt
void writeToFile(char *str) {
    FILE *write;
    write = fopen("file.txt", "a");
    if (write == NULL) {
        perror("can't create/open file");
        exit(EXIT_FAILURE);
    } else {
        fprintf(write, "%s\n", str);
        fclose(write);
    }
}

const char *count(char str[], int *charCount, int *wordCount) {
    int i = 0;
    int len = (int) strlen(str);
    char word[7];
    int wordInd = 0;

    while (i < len) {
        if (str[i] != ' ') {//if the char is not ' ' so we have to count i
            (*charCount)++;
            if ((*wordCount) < 1 && (*charCount) <
                                    8) { // if we are in the first word we enter the first 8 chars to the word to check if it "exit" or "history"
                word[wordInd] = str[i];
                wordInd++;
            }
        }//if the str[i] is ' ' and the next element isn't ' ' or '\0' so it's an legal char for a new word
        else if ((*charCount) != 0 && str[i + 1] != ' ' && str[i + 1] != '\0')
            (*wordCount)++;
        i++;
    }
    if ((*charCount) >
        0)//because we count the splits so the number of splits is more than 0, so there is (1+splits) words
        (*wordCount)++;
    word[wordInd] = '\0';// to finish the string if it less than 7 chars like "exit"
    if (strcmp(word, "done") == 0)
        return "done";
    if (strcmp(word, "nohup") == 0) {
        signal(SIGHUP, SIG_IGN);
        return "nohup";
    }
    if (strcmp(word, "history") == 0 && (*charCount) == 7)
        return "history";
    if (strcmp(word, "cd") == 0)
        return "cd";
    return "";
}


/*
 * this method execute the command using fork and execvp
 */
void ex(int WC, char *str, int *Total, int *commands) {
    writeToFile(str);
    char *arrayOfWords[WC + 1];
    splitToArray(arrayOfWords, str);
    if (strcmp(arrayOfWords[0], "nohup") == 0) {
        int fd = open("nohup.txt", O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (fd < 0) {
            perror("can't creat file");
        } else {
            printf("appending output to nohup.txt\n");
            int status;
            pid_t child;
            if ((child = fork()) == 0) {
                dup2(fd, STDOUT_FILENO);
                if (-1 == execvp(arrayOfWords[1], &arrayOfWords[1])) {
                    perror("command not supported (Yet)");
                    exit(EXIT_FAILURE);
                }
                exit(EXIT_SUCCESS);
            }
            if (child < 0) {
                perror("fork failed");
                exit(EXIT_FAILURE);
            } else {
                //wait for the child to end the process
                if (!BG) {
                    wait(&status);
                }
                close(fd);
                for (int j = 0; j < WC; j++)
                    free(arrayOfWords[j]);
                (*commands)++;
                (*Total) += WC - 1;
            }
        }
    } else {

        //create a new process to run the command
        pid_t x = fork();
        //if there's problem to create a new process
        if (x < 0) {
            perror("Fork unsuccessfully");
            exit(EXIT_FAILURE);
        }
        /*
         * if this the new process (child)
         * run the command using execvp method
         */
        int status;
        if (x == 0) {
            //check if the execvp doesn't work probably
            if (-1 == execvp(arrayOfWords[0], arrayOfWords)) {
                perror("command not supported (Yet)");
                exit(EXIT_FAILURE);
            }
            exit(EXIT_SUCCESS);
        } else {
            //wait for the child to end the process
            if (!BG) {
                wait(&status);
            }

            //after the finishing of execvp (child) we don't want the array of stings anymore, so we have to free it from the memory
            for (int j = 0; j < WC; j++)
                free(arrayOfWords[j]);
            //increase the command and sum the words at it
            (*commands)++;
            (*Total) += WC;

        }
    }
}

void freeArr3(char *arr1[], char *arr2[], char *arr3[], int N1, int N2, int N3) {
    for (int j = 0; j < N1; j++)
        free(arr1[j]);
    for (int j = 0; j < N2; j++)
        free(arr2[j]);
    for (int j = 0; j < N3; j++)
        free(arr3[j]);
}

void freeArr2(char *arr1[], char *arr2[], int N1, int N2) {
    for (int j = 0; j < N1; j++)
        free(arr1[j]);
    for (int j = 0; j < N2; j++)
        free(arr2[j]);
}

/*
 * split the string to an array and the last element is NULL, so the command will be ready to execvp method after this method
 */
void splitToArray(char *splitArray[], char Str[]) {
    int i = 0, start = 0, end = 0, j = 0, l = 0;
    if (Str[strlen(Str) - 1] == '&' && BG)
        Str[strlen(Str) - 1] = '\0';
    //replace quotation mark to space
    while (l < strlen(Str)) {
        if (Str[l] == '\"')
            Str[l] = ' ';
        l++;
    }
    while (i < strlen(Str) + 1) {
        //if the end change, so we arrived to the last char at the word
        if (end != 0) {
            // +1 for the '\0'
            int length = (end - start + 1);
            //every element in the array we need to allocate it exactly  to size of the word
            splitArray[j] = (char *) calloc(length, sizeof(char));
            //if we cannot allocate the memory, so we have to free every thing we allocate it and exit from the program
            if (splitArray[j] == NULL) {
                for (int k = 0; k < j; ++k)
                    free(splitArray[k]);
                perror("error can't allocate space in memory");
                exit(EXIT_FAILURE);
            } else {
                //copy the word to the element into the array
                strncpy(splitArray[j], &Str[start], length);
                j++;
            }
            end = 0;
        }
        //check if it's the first char at the word
        if (i > 0 && Str[i] != ' ' && Str[i - 1] == ' ')
            start = i;
        //check if it's the last char at the word
        if (Str[i] != ' ' && (Str[i + 1] == ' ' || Str[i + 1] == '\0'))
            end = i;

        i++;
    }
    if (strlen(Str) == 1) {
        splitArray[0] = (char *) calloc(2, sizeof(char));
        strcpy(splitArray[0], &Str[0]);
        j++;
    }
    //add the last element NULL for execvp method
    splitArray[j] = NULL;
}