#include <string>
#include <stdio.h>
#include <string.h>
#include <sys/sem.h>


void PrintHelp(const char * program) {
  printf("%s node_name [options] params \n", program);
  exit(-1);
}

int main(int argc, char* argv[])
{
    if(argc < 2) PrintHelp(argv[0]);

    // - Organizational parameters
    std::string nodeName = argv[1];
    std::string params;
    for(int i = 2; i < argc; ++i)
        params += std::string(argv[i]) + " ";
    std::string cmdline = std::string("/bin/bash ") + nodeName + " " + params;

    // - Invoke system commands
    system(cmdline.c_str());

	  return 0;
}
