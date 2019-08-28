/*
* 시스템 프로그래밍
* Simple shell 프로그램 (myshell)
* 
*/

#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#define COMMAND_MAXSIZE 1024
#define TOKEN_NUM 1024

///// 명령어 토큰을 하나씩 테스트 출력
/*void testCmdPrint(char **c, int size)
{
    int i;
    for (i = 0; i < size; i++)
    {
        printf("cmd[%d]: %s  ", i, c[i]);
    }
    printf("\n");
}*/

///// 백그라운드 수행 처리
int checkBackJob(char *cmdTokens[], int cmdIdxTmp)
{
    int backGroundTmp = 0;
    if (strcmp(cmdTokens[cmdIdxTmp - 1], "&") == 0)
    {
        //printf("&수행");
        cmdTokens[cmdIdxTmp - 1] = NULL;
        backGroundTmp = 1;
    }
    return backGroundTmp;
}

///// -c옵션에서 사용. 별다른 파이프,IO리다이렉션 없이 명령어 수행
void forkExeOneCmd(char *fname)
{
    int status;
    char *pathName = NULL;
    char *cmdPtr = NULL;
    char *cmd[TOKEN_NUM] = {
        NULL,
    };
    int cmdIdx = 0;

    ///// 입력 명령문 토큰화 시작 /////
    cmdPtr = strtok(fname, " ");
    while (cmdPtr != NULL) // 자른 문자열이 나오지 않을 때까지 반복
    {
        cmd[cmdIdx] = cmdPtr; // 문자열을 자른 뒤 메모리 주소를 문자열 포인터 배열에 저장
        cmdIdx++;             // 인덱스 증가
        cmdPtr = strtok(NULL, " "); // 다음 문자열을 잘라서 포인터를 반환
    }
    pathName = (char *)malloc(sizeof(char) * strlen(cmd[0]));
    strcpy(pathName, cmd[0]);
    ///// 입력 명령문 토큰화 완료 /////

    execvp(pathName, cmd);

    free(pathName);
    exit(0);
}

///// 명령어에 |가 몇개나 있는지 구함
int getPipeNums(char *s)
{
    int length = strlen(s);
    int counts = 0;

    int i = 0;
    for (i = 0; i < length; i++)
    {
        if (s[i] == '|')
        {
            counts++;
        }
    }

    return counts;
}

///// 파이프 사용되는 명령어 처리
void pipeShellCmd(char *inputCmd)
{
    int statusTmp, waitValTmp = 0;
    int pipeFd[TOKEN_NUM][2];
    char *firstFileForPipe = NULL;
    char *nextFileForPipe = NULL;
    int i;

    int pipeIdx = 0;
    int pipeNums = getPipeNums(inputCmd);

    ///// 파이프 생성 /////
    for (i = 0; i < pipeNums; i++)
    {
        pipe(pipeFd[i]);
    }

    ///// 파이프 |로 토큰화 시작 /////
    firstFileForPipe = strtok(inputCmd, "|");
    while (firstFileForPipe != NULL)
    {
        nextFileForPipe = strtok(NULL, "|");

        //처음 파트 명령 수행
        int child_pid0 = fork();
        if (child_pid0 == 0) //child
        {
            ///// 입력 명령문 토큰화 시작 /////
            char *pathName = NULL;
            char *cmdPtr = NULL;
            int cmdIdx = 0;
            char *cmd[TOKEN_NUM] = {
                NULL,
            };
            cmdPtr = strtok(firstFileForPipe, " ");
            while (cmdPtr != NULL) // 자른 문자열이 나오지 않을 때까지 반복
            {
                cmd[cmdIdx] = cmdPtr;       // 문자열을 자른 뒤 메모리 주소를 문자열 포인터 배열에 저장
                cmdIdx++;                   // 인덱스 증가
                cmdPtr = strtok(NULL, " "); // 다음 문자열을 잘라서 포인터를 반환
            }
            pathName = (char *)malloc(sizeof(char) * strlen(cmd[0]));
            strcpy(pathName, cmd[0]);
            ///// 입력 명령문 토큰화 완료 /////

            if (pipeIdx == 0)
            {
                //첫번째 명령어
            }
            else //두번째 명령어부터
            {
                fclose(stdin);
                dup(pipeFd[pipeIdx - 1][0]);
                close(pipeFd[pipeIdx - 1][0]);
            }

            if (nextFileForPipe != NULL)
            {
                fclose(stdout);
                dup(pipeFd[pipeIdx][1]);
                close(pipeFd[pipeIdx][1]);
            }

            //dup처리했으니 파이프들 닫음
            for (i = 0; i < pipeNums; i++)
            {
                close(pipeFd[i][0]);
                close(pipeFd[i][1]);
            }

            execvp(pathName, cmd);
            free(pathName);
            //exit(0);
        }
        else if (child_pid0 < 0)
        {
            printf("에러 child! 명령어= %s \n", firstFileForPipe);
            exit(1);
        }
        else
        {
            firstFileForPipe = nextFileForPipe;
            pipeIdx += 1;
        }
    }
    //파이프 닫음
    for (i = 0; i < pipeNums; i++)
    {
        close(pipeFd[i][0]);
        close(pipeFd[i][1]);
    }
    //사용된 커맨드 처리하는 자식 프로세스 처리
    for (i = 0; i < pipeNums + 1; i++)
    {
        int pid = wait(&statusTmp);
    }
    return;
}

void printPrompt()
{
    printf("$ ");
}

void normalShellCmd()
{
    int status, waitVal;
    int isBackGround;
    char input_cmd[COMMAND_MAXSIZE];
    char input_cmdBackUp[COMMAND_MAXSIZE];
    char *outFileName = NULL;
    char *inFileName = NULL;
    char *errFileName = NULL;
    char *pathName = NULL;
    char *cmdPtr = NULL;

    while (1)
    {
        int i = 0;
        int cmdIdx = 0;
        waitVal = 0;
        isBackGround = 0;
        memset(input_cmd, 0, sizeof(input_cmd));
        memset(input_cmdBackUp, 0, sizeof(input_cmdBackUp));
        outFileName = NULL;
        inFileName = NULL;
        errFileName = NULL;
        pathName = NULL;
        cmdPtr = NULL;
        char *cmd[TOKEN_NUM] = {
            NULL,
        };

        printPrompt(); // print “$”

        fgets(input_cmd, sizeof(input_cmd), stdin);
        input_cmd[strlen(input_cmd) - 1] = '\0';
        if (feof(stdin))
        {
            printf("Ctrl-D\n");
            exit(0);
        }
        strcpy(input_cmdBackUp, input_cmd); //명령어 원본 백업

        ///// 입력 명령문 토큰화 시작 /////
        cmdPtr = strtok(input_cmd, " ");
        while (cmdPtr != NULL) // 자른 문자열이 나오지 않을 때까지 반복
        {
            cmd[cmdIdx] = cmdPtr;       // 문자열을 자른 뒤 메모리 주소를 문자열 포인터 배열에 저장
            cmdIdx++;                   // 인덱스 증가
            cmdPtr = strtok(NULL, " "); // 다음 문자열을 잘라서 포인터를 반환
        }
        pathName = (char *)malloc(sizeof(char) * strlen(cmd[0]));
        strcpy(pathName, cmd[0]);
        ///// 입력 명령문 토큰화 완료 /////

        ///// 파이프 수행 처리
        int pipeNums = getPipeNums(input_cmdBackUp);
        if (pipeNums > 0)
        {
            pipeShellCmd(input_cmdBackUp);
            continue;
        }

        int child_pid = fork();
        if (child_pid == 0)
        {
            ///// 백그라운드 수행 처리
            if ((isBackGround = checkBackJob(cmd, cmdIdx)) == 1)
            {
                cmd[cmdIdx - 1] = "\0";
            }

            ///// IO리다이렉션 >,< 처리(multiple 완료)
            int boundCmdIdx = -1;
            for (i = 0; i < cmdIdx; i++)
            {
                if (strcmp(cmd[i], ">") == 0)
                {
                    if (boundCmdIdx == -1)
                    {
                        boundCmdIdx = i;
                    }

                    outFileName = cmd[i + 1];
                    int outFileFd = open(outFileName, O_WRONLY | O_CREAT /*| O_TRUNC*/, 0644);
                    fclose(stdout);
                    dup(outFileFd);
                    close(outFileFd);
                }
                if (strcmp(cmd[i], "<") == 0)
                {
                    if (boundCmdIdx == -1)
                    {
                        boundCmdIdx = i;
                    }

                    inFileName = cmd[i + 1];
                    int inFileFd = open(inFileName, O_RDONLY /*| O_CREAT*/);
                    fclose(stdin);
                    dup(inFileFd);
                    close(inFileFd);
                }
                if (strcmp(cmd[i], "2>") == 0)
                {
                    if (boundCmdIdx == -1)
                    {
                        boundCmdIdx = i;
                    }

                    errFileName = cmd[i + 1];
                    int errFileFd = open(errFileName, O_WRONLY | O_CREAT, 0644);
                    fclose(stderr);
                    dup(errFileFd);
                    close(errFileFd);
                }
            }

            //>,<발생 시점부터 cmd배열 초기화 -> 쓸모있는 커맨드만 남김
            for (i = boundCmdIdx; i < cmdIdx && boundCmdIdx != -1; i++)
            {
                cmd[i] = NULL;
            }

            if (isBackGround == 1)
            {
                child_pid = fork();
                if (child_pid == 0)
                {
                    execvp(pathName, cmd);
                    exit(0);
                }
                else
                {
                    exit(0);
                }
            }
            else
            {
                execvp(pathName, cmd);
            }
        }
        else
        {
            waitpid(child_pid, &status, waitVal);
        }
    }

    free(pathName);
}

void cOptionShellCmd(char **argv)
{
    int statusTmp, waitValTmp = 0;
    char *pathName = NULL;
    char *argvString = NULL;
    char *cmdPtr = NULL;
    char tmpCmd[COMMAND_MAXSIZE];

    cmdPtr = strtok(argv[2], ";");
    while (cmdPtr != NULL)
    {
        memset(tmpCmd, 0, sizeof(tmpCmd));
        strcpy(tmpCmd, cmdPtr);

        int child_pid = fork();
        if (child_pid == 0)
        {
            forkExeOneCmd(tmpCmd);
        }
        else
        {
            waitpid(child_pid, &statusTmp, waitValTmp);
        }
        cmdPtr = strtok(NULL, ";"); // 다음 문자열(string)을 잘라서 포인터를 반환
    }
}

int main(int argc, char **argv)
{
    if (argc > 1 && (strcmp(argv[1], "-c") == 0))
    {
        cOptionShellCmd(argv);
    }
    else
    {
        normalShellCmd();
    }
    return 0;
}