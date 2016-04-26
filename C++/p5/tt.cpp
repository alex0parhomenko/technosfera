#include <iostream>
#include <algorithm>
#include <unistd.h>
#include <string>

using namespace std;

int main()
{
	int fd[40];
	pipe(fd);
	pipe(fd + 2);
	cout << fd[0] << " " << fd[1] << " " << fd[2] << " " << fd[3] << endl;
	return 0;
}