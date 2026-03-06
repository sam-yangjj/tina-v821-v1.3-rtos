#include <iostream>
#include <console.h>

#include "func_test.h"

using namespace std;

void cpp_native_func(void)
{
	cout << "[" __FILE__ ", " << __LINE__ << "] " << "C++ native function" << endl;
	int array[4] = { 1, 2, 3, 4};
	int index = 0;

	//C++11 new feature: for loop based on range
	for (auto item : array)
	{
		cout << "array[" << index << "] = "<< item << endl;
		index++;
	}
}

int cmd_cpp_test(int argc, char *argv[])
{
	cout << "Call C++ native function in C++ source:" << endl;
	cpp_native_func();
	cout << endl;

	cout << "Call C native function in C++ source:" << endl;
	c_native_func();
	cout << endl;

	cout << "Call C++ native function in C source:" << endl;
	call_cpp_func_in_c();
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_cpp_test, cpp_test, C++ environment test);
