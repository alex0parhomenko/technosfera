#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <limits.h>
#include <time.h>

using namespace std;

#define f first
#define s second
#define mp make_pair
#define file_name "in.bin"

long long get_rand(long long low_bound, long long up_bound)
{
	double x = ((double)rand() / (double)RAND_MAX) * ((double)up_bound - (double)low_bound) + low_bound;
	return (long long) x;
}

void print_example(long long doc1, vector <long long> v)
{
	//sort(v.begin(), v.end());
	auto it = unique(v.begin(), v.end());
	v.resize(distance(v.begin(), it));
	sort(v.begin(), v.end());
	cout << doc1 << " " << v.size() << " ";
	for (auto i = 0; i < v.size(); i++)
	{
		cout << v[i];
		if (i != v.size() - 1)
			cout << " ";
	}
	cout << "\n";
}

int main(int argc, char * argv[])
{
	srand(time(NULL));
	auto cou_examples = 10;
	auto fd_out = open(file_name, O_RDWR);
	vector <pair <long long, vector <long long> > > index;
	for (auto i = 0; i < cou_examples; i++)
	{
		vector <long long> v1;
		long long doc1 = get_rand(1, 50);
		int cou = (int)get_rand(1, 30);
		for (auto j = 0; j < cou; j++)
			v1.push_back(get_rand(1, 20));

		write(fd_out, &doc1, sizeof(doc1));
		write(fd_out, &cou, sizeof(cou));
		for (auto j = 0; j < v1.size(); j++)
		{
			write(fd_out, &(v1[j]), sizeof(v1[j]));
		}
		print_example(doc1, v1);
	}
	close(fd_out);
	return 0;
}