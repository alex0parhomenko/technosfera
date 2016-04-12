#include <iostream>
#include <algorithm>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <map>

#define f first
#define s second
using namespace std;

int main(int argc, char * argv[])
{
	auto fd = open("result.bin", O_RDWR);
	/*auto fd_out = open("out.bin", O_RDONLY);
	lseek(fd_out, 784, SEEK_SET);
	long long a = 0, c = 0;
	int b = 0;
	read(fd_out, &a, sizeof(a));
	read(fd_out, &b, sizeof(b));
	read(fd_out, &c, sizeof(c));
	cout << a << " " << b << " " << c << endl;
	return 0;*/
	/*auto fd_out = open("out.bin", O_RDWR);
	lseek(fd_out, 1360, SEEK_SET);
	long long w = 0;
	read(fd_out, &w, sizeof(w));
	cout << w << endl;

	return 0;*/
	long long w = 0, offset = 0, doc = 0;
	int k = 0, cou = 0;
	map <long long, vector<long long> > d;
	int now_offset = 0;
	while (true)
	{
		k++;
		read(fd, &w, sizeof(w));
		now_offset += sizeof(long long);
		//cout << "word is " << w << " ";
		read(fd, &offset, sizeof(offset));
		now_offset += sizeof(long long);
		if (w == 0 && offset == 0)
			break;
		lseek(fd, offset, SEEK_SET);
		read(fd, &cou, sizeof(int));
		//cout << cou << endl;
		for (auto i = 0; i < cou; i++)
		{
			read(fd, &doc, sizeof(long long));
			//cout << doc << " ";
			if (d.count(doc) != 0)
			{
				d[doc].push_back(w);
			}
			else
			{
				vector <long long> v;
				v.push_back(w);
				d[doc] = v;
			}
		}
		lseek(fd, now_offset, SEEK_SET);
		//cout << endl;
	}
	//cout << w << " " << offset << endl;
	auto beg = d.begin();
	auto end = d.end();
	while (beg != end)
	{
		auto doc = (*beg).f;
		auto v = (*beg).s;
		auto it = unique(v.begin(), v.end());
		v.resize(distance(v.begin(), it));
		cout << "docid: " << doc;
		cout << ", words:" << endl;
		cout << "cnt: " << v.size() << endl; 
		for (auto i = 0; i < v.size(); i++)
		{
			cout << v[i] << "; ";
		}
		cout << endl;	
		cout << endl;
		beg++;
	}
	return 0;	
}