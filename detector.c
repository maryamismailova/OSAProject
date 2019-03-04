#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <time.h>
#include <sys/mman.h>

#define MAXSIZE 4096
bool printReturnCode=false;
bool timeFormatSet=false;
bool limitSet=false;
bool intervalSet=false;

int interval=10000;//in millis
char timeFormat[100];
int limitOfIterations=0;//0 for infinity

char *previousOutput;

int runCommand(char *command);

int runCommand(char *command){
    int outputSize=MAXSIZE+1;
    char *outputString=calloc(outputSize, sizeof(char));
    memset(outputString, 0, MAXSIZE);
    int pipes[2];
    if(pipe(pipes)==-1){
        perror("Pipe");
        exit(0);
    }
    int pid=fork();
    if(pid==0){
        dup2(pipes[1], 1);
        close(pipes[0]);
        close(pipes[1]);
        execlp("/bin/sh","sh", "-c", command, NULL);
        // exit(5);
    }else if(pid==-1){
        perror("Fork");
        exit(0);
    }else
    {   close(pipes[1]);
        char *readChar=calloc(1024+1, sizeof(char));
        int nbBytes=0;
        int nbRead=0;
        while((nbBytes=read(pipes[0], readChar, 1024))>0){
            nbRead+=nbBytes;
            if(nbRead>outputSize){
                outputSize=nbRead+1;
                outputString=realloc(outputString, outputSize);
            }
            strcat(outputString, readChar);
            memset(readChar, 0, 1025);
        }
        
        int status;
        wait(&status);
        if(WIFEXITED(status)){
            // printf("child exits with status %d\n", WEXITSTATUS(status));
            if(strcmp(previousOutput, outputString)==0){
                // printf("child exits with status %d\n", WEXITSTATUS(status));
                // printf("The same output!\n");
            }else{
                previousOutput=calloc(outputSize, sizeof(char));
                strcpy(previousOutput, outputString);
                printf("\nthe output is\n %s\n", outputString);
            }
        }
        return WEXITSTATUS(status);
    }   
}
int main(int argc, char *argv[])
{
    // fclose(stderr);
    int progStartIndex=-1;
    int opt=-1;
    char **argv2=calloc(argc, sizeof(char *));
    for(int i=0;i<argc;i++){
        argv2[i]=calloc(100, sizeof(char));
        strcpy(argv2[i], argv[i]);
    }
    while((opt=getopt(argc, argv, "+ci:t:l:"))!=-1){
        switch (opt)
        {
            case 'c':
                if(printReturnCode==false){
                    printReturnCode=true;
                    printf("Return code true\n");
                }
                break;
            case 'i':
                if(intervalSet==false){
                    interval=atoi(argv[optind-1]);
                    intervalSet=true;
                    printf("Set interval to %d\n", interval);
                }
                break;
            case 't':
                if(timeFormatSet==false){
                    timeFormatSet=true;
                    strcpy(timeFormat, argv[optind-1]);
                    printf("Set time format to %s\n", timeFormat);
                }
                break;
            case 'l':
            if(limitSet==false){
                limitSet=true;
                limitOfIterations=atoi(argv[optind-1]);
                printf("Set limit to %d\n", limitOfIterations);
            }
                break;
            case '?':
                printf("Option is needed\n");
                break;            
            default:
                printf("default\n");
                break;
        }
    }
    // printf("optind %d\n", optind);
    progStartIndex=optind;
    int length=argc-progStartIndex;
    char command[256];
    for(int i=0;i<length;i++){
        sprintf(command, "%s %s", command, argv2[i+progStartIndex]);
    }
    printf("command: %s\n", command);

    previousOutput=calloc(MAXSIZE+1, sizeof(char));
    int counter=0;
    while(counter<limitOfIterations || limitSet==false){
        if(timeFormatSet){
            char currentTime[100];
            time_t curTime=time(NULL);
            struct tm* locTime=localtime(&curTime);
            strftime(currentTime, 100, timeFormat, locTime);
            printf("%s\n", currentTime);
        }
        int exitStatus=runCommand(command);
        if(counter==0){
            printf("exit %d\n", exitStatus);
            }
        // else printf(" ");
        usleep(interval*1000);
        counter++;
    }
    return 0;
}