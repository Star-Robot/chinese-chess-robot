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

void callback(std_msgs::Int8::ConstPtr msg)
{}

int main()
{
    Node node("hlbnode");
    Publisher<std_msgs::Int8>::Ptr pub =
        node.Advertise<std_msgs::Int8>("int8_array", 10);
    Subscriber<std_msgs::Int8>::Ptr sub =
        node.Subscribe<std_msgs::Int8>("int8_array", 10, callback);
    return 0;
}
