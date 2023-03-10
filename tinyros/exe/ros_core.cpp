#include "base/core.hpp"

void RunCore()
{
	tinyros::Core core;
	core.Start();
}


int main()
{
	RunCore();
	return 0;
}
