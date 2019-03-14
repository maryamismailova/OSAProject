#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/mman.h>
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
int prevOutputSize=0;

int runCommand(char *command);

void usage(char *progname){
    fprintf(stderr, "usage:\n %s [-t format ] [-i interval ] [ -l limit ] [-c ] prog arg ...arg\n", progname);
    fprintf(stderr, " -t\tprinting date and time for each launch, using specified format( strftime() )\n");
    fprintf(stderr, " -i\tthe interval (in milliseconds) between to program launch\n");
    fprintf(stderr, " -l\tthe limit of launch number or 0 for no limit\n");
    fprintf(stderr, " -c\tdetects modifications of return code\n");
}

int runCommand(char *command){
    int outputSize=MAXSIZE;
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
        char *readChar=calloc(1024, sizeof(char));
        if(readChar==NULL){
            perror("Calloc");
            exit(0);
        }
        int nbBytes=0;
        int nbRead=0;
        while((nbBytes=read(pipes[0], outputString+nbRead, 1024))!=0){
            nbRead+=nbBytes;
            if(nbRead>=outputSize){
                outputSize=nbRead+1024;
                outputString=realloc(outputString, outputSize);
                if(outputString==NULL){
                    perror("Realloc");
                    exit(0);
                }
            }
        }
        int status;
        wait(&status);
        if(WIFEXITED(status)){
            if(memcmp(previousOutput, outputString, nbRead)==0){
            }else{
                previousOutput=calloc(nbRead, sizeof(char));
                prevOutputSize=nbRead;
                if(previousOutput==NULL){
                    perror("Calloc");
                    exit(0);
                }
                memcpy(previousOutput, outputString,nbRead);
                // printf("%s", outputString);
                write(1, outputString, nbRead);
            }
        }
        return WEXITSTATUS(status);
    }   
}
int main(int argc, char *argv[])
{
    if(argc==1){
        usage(argv[0]);
        exit(1);
    }
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
                    // printf("Return code true\n");
                }
                break;
            case 'i':
                if(intervalSet==false){
                    // interval=atoi(argv[optind-1]);
                    interval=atoi(optarg);
                    if(interval <=0){
                        usage(argv[0]);
                        exit(1);
                    }
                    intervalSet=true;
                    // printf("Set interval to %d\n", interval);
                }
                break;
            case 't':
                if(timeFormatSet==false){
                    timeFormatSet=true;
                    // strcpy(timeFormat, argv[optind-1]);
                    strcpy(timeFormat, optarg);
                    // printf("Set time format to %s\n", timeFormat);
                }
                break;
            case 'l':
            if(limitSet==false){
                limitSet=true;
                limitOfIterations=atoi(optarg);
                if(limitOfIterations<0){
                    usage(argv[0]);
                    exit(1);
                }
            }
                break;
            case '?':
                usage(argv[0]);
                exit(1);
                break;            
            default:
                usage(argv[0]);
                exit(1);
                break;
        }
    }
    progStartIndex=optind;
    if(optind==argc){
        usage(argv[0]);
        exit(1);
    }
    int length=argc-progStartIndex;
    char command[256];
    for(int i=0;i<length;i++){
        sprintf(command, "%s %s", command, argv2[i+progStartIndex]);
    }

    previousOutput=calloc(MAXSIZE, sizeof(char));
    prevOutputSize=MAXSIZE;
    if(previousOutput==NULL){
        perror("Calloc");
        exit(1);
    }
    int counter=0;
    while(counter<limitOfIterations || limitSet==false){
        if(timeFormatSet){
            char currentTime[100];
            time_t curTime=time(NULL);
            struct tm* locTime=localtime(&curTime);
            strftime(currentTime, 100, timeFormat, locTime);
            sprintf(currentTime, "%s\n", currentTime);
            write(1, currentTime, strlen(currentTime));
        }
        int exitStatus=runCommand(command);
        if(printReturnCode==true){
                char exString[100];
                sprintf(exString , "exit %d\n", exitStatus);
                write(1, exString, strlen(exString));
        }
        usleep(interval*1000);
        counter++;
    }
    return 0;
}