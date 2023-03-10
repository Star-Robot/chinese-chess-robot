#ifndef PIPE_H
#define PIPE_H

const int LINE_INPUT_MAX_CHAR = 1024;
const int PATH_MAX_CHAR = 1024;
const int PATH_SEPARATOR = '/';

struct PipeStruct
{
    int nInput, nOutput;
    int nEof;
    int nReadEnd;
    char szBuffer[LINE_INPUT_MAX_CHAR];

    void ParseDir(char *szDir, const char *szPath);
    void Open(const char *szExecFile = NULL);
    void Close(void) const;
    void ReadInput(void);
    bool CheckInput(void);
    bool GetBuffer(char *szLineStr);
    bool LineInput(char *szLineStr);
    void LineOutput(const char *szLineStr) const;
}; // pipe

#endif
