#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <fcntl.h>
#include <string.h>
#include <algorithm>
#include <string>
#include <omp.h>
#include <time.h>

using namespace std;

void merge_sort(int v[], int beg, int end, bool mode, int tmp[], int num_threads)
{
	if (beg == end)
		return;
	auto pos = (end - beg) / 2;
	if (mode && num_threads > 0)
	{
		#pragma omp parallel sections num_threads(2)
		{
			#pragma omp section
				merge_sort(v, beg, beg + pos, mode, tmp, num_threads / 2);
			#pragma omp section
				merge_sort(v, beg + pos + 1, end, mode, tmp, num_threads - num_threads / 2);
		}
	}
	else
	{
		merge_sort(v, beg, beg + pos, mode, tmp, num_threads);
		merge_sort(v, beg + pos + 1, end, mode, tmp, num_threads);	
	}
	auto pos1 = beg;
	auto pos2 = beg + pos + 1;
	auto it = beg;

	while (pos1 < beg + pos + 1 && pos2 < end + 1)
	{
		if (v[pos1] < v[pos2])
		{
			tmp[it] = v[pos1];
			pos1++;
		}
		else
		{
			tmp[it] = v[pos2];
			pos2++;
		}
		it++;
	}
	while (pos1 < beg + pos + 1)
	{
		tmp[it] = v[pos1];
		pos1++;
		it++;
	}
	while (pos2 < end + 1)
	{
		tmp[it] = v[pos2];
		pos2++;
		it++;
	}
	memcpy(v + beg, tmp + beg, (end - beg + 1) * sizeof(int));
}


int main(int argc, char * argv[])
{
	int num_threads = 0;
	#pragma omp parallel
	{
		#pragma omp master
		{
			num_threads = omp_get_num_threads();
		}
	}
	auto SIZE = 10000000;
	srand(time(NULL));
	time_t t1, t2, t3;
	int * a = new int[SIZE];
	int * b = new int[SIZE];
	int * c = new int[SIZE];
	int * tmp = new int[SIZE];

	for (int i = 0; i < SIZE; i++)
	{
		auto val = rand() / 200;
		a[i] = val;
		b[i] = val;
		c[i] = val;
	}
	time(&t1);
	merge_sort(a, 0, SIZE - 1, true, tmp, num_threads);
	time(&t2);
	merge_sort(c, 0, SIZE - 1, false, tmp, 0);
	time(&t3);
	sort(b, b + SIZE);
	bool flag = true;
	for (auto i = 0; i < SIZE; i++)
		if (a[i] != b[i])
			flag = false;
	if (flag)
		cout << "equal" << endl;
	else
		cout << "not equal" << endl;
	cout << "Parallel sort: " << t2 - t1 << "\nSynchronic sort: " << t3 - t2 << endl;

	delete [] a;
	delete [] b;
	delete [] c;

	return 0;
}