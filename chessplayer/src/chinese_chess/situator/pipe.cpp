#include <iostream>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/select.h>
#include <time.h>
#include "pipe.hpp"

/***
 *
 * This code copy heavily from
 * https://github.com/xqbase/eleeye/blob/master/UCCI2QH/ucci2qh.cpp
 * and https://github.com/xqbase/eleeye/blob/master/TEST/uccitest.cpp
 */

void PipeStruct::ParseDir(char *szDir, const char *szPath)
{
    char *lpSeparator;
    strcpy(szDir, szPath);
    lpSeparator = strrchr(szDir, PATH_SEPARATOR);
    if (lpSeparator == NULL) {
        szDir[0] = '\0';
    } else {
        *lpSeparator = '\0';
    }
}


void PipeStruct::Open(const char *szProcFile)
{
    int nStdinPipe[2], nStdoutPipe[2];
    char szDir[PATH_MAX_CHAR];

    nEof = 0;
    if (szProcFile == NULL)
    {
        nInput = STDIN_FILENO;
        nOutput = STDOUT_FILENO;
    }
    else
    {
        pipe(nStdinPipe);
        pipe(nStdoutPipe);
        if (fork() == 0)
        {
            close(nStdinPipe[1]);
            close(nStdoutPipe[0]);
            dup2(nStdinPipe[0], STDIN_FILENO);
            close(nStdinPipe[0]);
            dup2(nStdoutPipe[1], STDOUT_FILENO);
            dup2(nStdoutPipe[1], STDERR_FILENO);
            close(nStdoutPipe[1]);

            ParseDir(szDir, szProcFile);
            chdir(szDir);

            nice(20);
            execl(szProcFile, szProcFile, NULL);
            exit(EXIT_FAILURE);
        }
        close(nStdinPipe[0]);
        close(nStdoutPipe[1]);
        nInput = nStdoutPipe[0];
        nOutput = nStdinPipe[1];
    }
    nReadEnd = 0;
}


void PipeStruct::Close(void) const
{
    close(nInput);
    close(nOutput);
}


void PipeStruct::ReadInput(void)
{
    int n = read(nInput, szBuffer + nReadEnd, LINE_INPUT_MAX_CHAR - nReadEnd);
    if (n < 0) {
        nEof = 1;
    } else {
        nReadEnd += n;
    }
}


bool PipeStruct::CheckInput(void)
{
    fd_set set;
    timeval tv;
    int val;
    FD_ZERO(&set);
    FD_SET(nInput, &set);
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    val = select(nInput + 1, &set, NULL, NULL, &tv);
    return (val > 0 && FD_ISSET(nInput, &set) > 0);
}


void PipeStruct::LineOutput(const char *szLineStr) const
{
    int nStrLen;
    char szWriteBuffer[LINE_INPUT_MAX_CHAR];
    nStrLen = strlen(szLineStr);
    memcpy(szWriteBuffer, szLineStr, nStrLen);
    szWriteBuffer[nStrLen] = '\n';
    write(nOutput, szWriteBuffer, nStrLen + 1);
}


bool PipeStruct::GetBuffer(char *szLineStr)
{
    char *lpFeedEnd;
    int nFeedEnd;
    lpFeedEnd = (char *) memchr(szBuffer, '\n', nReadEnd);
    if (lpFeedEnd == NULL) return false;

    nFeedEnd = lpFeedEnd - szBuffer;
    memcpy(szLineStr, szBuffer, nFeedEnd);
    szLineStr[nFeedEnd] = '\0';
    nFeedEnd ++;
    nReadEnd -= nFeedEnd;
    memcpy(szBuffer, szBuffer + nFeedEnd, nReadEnd);
    lpFeedEnd = (char *) strchr(szLineStr, '\r');
    if (lpFeedEnd != NULL)
    {
        *lpFeedEnd = '\0';
    }
    return true;
}


bool PipeStruct::LineInput(char *szLineStr)
{
    if (GetBuffer(szLineStr)) {
        return true;
    } else if (CheckInput()) {
        ReadInput();
        if (GetBuffer(szLineStr)) {
            return true;
        } else if (nReadEnd == LINE_INPUT_MAX_CHAR) {
            memcpy(szLineStr, szBuffer, LINE_INPUT_MAX_CHAR - 1);
            szLineStr[LINE_INPUT_MAX_CHAR - 1] = '\0';
            szBuffer[0] = szBuffer[LINE_INPUT_MAX_CHAR - 1];
            nReadEnd = 1;
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}
