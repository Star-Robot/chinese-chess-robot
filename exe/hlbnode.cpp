#include <string>
#include <stdio.h>
#include <string.h>
#include <sys/sem.h>
#include "base/node.hpp"
#include "base/publisher.hpp"
#include "base/subscriber.hpp"
#include "message/std_msgs.hpp"

using namespace huleibao;

void PrintHelp(const char * program) {
  printf("%s node_name [options] params \n", program);
  exit(-1);
}

// template class Publisher<std_msgs::Int8>;
// template class Subscriber<std_msgs::Int8>;
int main()
{
    Node node("hlbnode");
    std_msgs::Int8 msg;
    //Publisher<std_msgs::Int8>::Ptr pub =
    //    node.Advertise<std_msgs::Int8>("int8_array", 10);
    return 0;
}
