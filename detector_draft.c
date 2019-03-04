#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>

#define MAXSIZE 4096
bool printReturnCode=false;
bool timeFormatSet=false;
bool limitSet=false;
bool intervalSet=false;

int interval=10000;//in millis
char timeFormat[100];
int limitOfIterations=0;//0 for infinity
char *previousOutput;
unsigned long *modeTime;//store modification time of all files

int runCommand(char *command);

unsigned long *readModeTime(char *name){
    unsigned long *timeMode=calloc(1000, sizeof(unsigned long));
    struct stat times;
    DIR *d;
    struct dirent *dir;
    int index=0;
    d=opendir(name);
    while( (dir=readdir(d))!=NULL ){
        stat(dir->d_name, &times);
        timeMode[index++]=times.st_mtime;
    }
    return timeMode;
}

int runCommand(char *command){
    int outputSize=MAXSIZE+1;
    char *outputString=calloc(outputSize, sizeof(char));
    if(outputString==NULL){
        perror("calloc");
        exit(0);
    }
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
    }else if(pid==-1){
        perror("Fork");
        exit(0);
    }else
    {   close(pipes[1]);
        char *readChar=calloc(1024+1, sizeof(char));
        if(readChar==NULL){
            perror("Calloc");
            exit(0);
        }
        int nbBytes=0;
        int nbRead=0;
        while((nbBytes=read(pipes[0], readChar, 1024))>0){
            nbRead+=nbBytes;
            if(nbRead>outputSize){
                outputSize=nbRead+1;
                outputString=realloc(outputString, outputSize);
                if(outputString==NULL){
                    perror("Realloc");
                    exit(0);
                }
            }
            strcat(outputString, readChar);
            memset(readChar, 0, 1025);
        }

        unsigned long *times;//[1000];        
        int status;
        wait(&status);
        if(WIFEXITED(status)){
            if(strcmp(previousOutput, outputString)==0){
                printf("\n");
            }else{
                previousOutput=calloc(outputSize, sizeof(char));
                if(previousOutput==NULL){
                    perror("Calloc");
                    exit(0);
                }
                strcpy(previousOutput, outputString);
                printf("%s\n", outputString);
                printf("exit %d\n", WEXITSTATUS(status));
            }
        }
        return WEXITSTATUS(status);
    }   
}
int main(int argc, char *argv[])
{
    int progStartIndex=-1;
    int opt=-1;
    char **argv2=calloc(argc, sizeof(char *));
    if(argv2==NULL){
        perror("Calloc");
        exit(0);
    }
    for(int i=0;i<argc;i++){
        argv2[i]=calloc(100, sizeof(char));
        if(argv2[i]==NULL){
            perror("Calloc");
            exit(0);
        }
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
    progStartIndex=optind;
    int length=argc-progStartIndex;
    char command[256];
    for(int i=0;i<length;i++){
        sprintf(command, "%s %s", command, argv2[i+progStartIndex]);
    }

    previousOutput=calloc(MAXSIZE+1, sizeof(char));
    if(previousOutput==NULL){
        perror("Calloc");
        exit(0);
    }
    int counter=0;
    while(counter<limitOfIterations || limitSet==false){
        if(timeFormatSet){
            char currentTime[100];
            time_t curTime=time(NULL);
            struct tm* locTime=localtime(&curTime);
            strftime(currentTime, 100, timeFormat, locTime);
            printf("%s\n", currentTime);
        }
        modeTime=calloc(1000, sizeof(unsigned long));
        modeTime=readModeTime(argv2[argc-1]);
        runCommand(command);
        usleep(interval*1000);
        counter++;
    }
    return 0;
}