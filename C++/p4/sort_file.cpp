#include <iostream>
#include <vector>
#include <stdlib.h>
#include <algorithm>
#include <map>
#include <cstdio>
#include <string.h>
#include <string>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sstream>
#include <fstream>
#include <limits.h>

#define byte8 8
#define byte4 4
#define simple_const 77
#define mp make_pair
#define f first
#define s second
#define mem_size 512
#define file_in_name "in.bin"
#define file_out_name "out.bin"
#define result_file "result.bin"

using namespace std;

class offset{
public:
	long long now_pos;
	long long end_pos;
	offset(long long now_pos, long long end_pos) : now_pos(now_pos), end_pos(end_pos) {}
};

vector <offset> offsets_in_file;
long long now_offset_out_file = 0;

void print_map_in_file(map<long long, vector <long long> > m, int fd)
{
	auto beg = m.begin();
	auto end = m.end();
	auto note_size = 0;
	while (beg != end)
	{

		long long word = beg->f;
		//cout << "word is: " << word << " docs is: "; 
		vector <long long> docs = beg->s;
		auto it = unique(docs.begin(), docs.end());
		docs.resize(distance(docs.begin(), it));
		sort(docs.begin(), docs.end());
		/*for (auto i : docs)
		{
			cout << i << " ";
		}*/
		int sz = docs.size();
		write(fd, &word, sizeof(word));
		write(fd, &sz, sizeof(sz));

		note_size += sizeof(word);
		note_size += sizeof(sz);

		for (auto i = 0; i < docs.size(); i++)
		{
			write(fd, &(docs[i]), sizeof(docs[i]));
			note_size += sizeof(docs[i]);
		}
		beg++;
		//cout << endl;
	}
	//cout << endl;
	offsets_in_file.push_back(offset(now_offset_out_file, now_offset_out_file + note_size));
	now_offset_out_file += note_size;

}

int main(int argc, char * argv[])
{
	auto fd_in = open(file_in_name, O_RDONLY);
	auto fd_out = open(file_out_name, O_WRONLY | O_RDONLY);
	auto fd_final = open(result_file, O_WRONLY);
	auto BUF_SIZE = mem_size / 2;
	char * buf = (char *)malloc(BUF_SIZE);
	
	long long doc_id = 0;
	int word_count = 0;
	long long now_word = 0;

	long long old_doc_id = 0;
	long long old_words_balance = 0;

	map <long long, vector <long long> > doc_words;

	auto start_pos = 0;
	auto count = 0;
	auto balance = BUF_SIZE - 1 - start_pos;
	auto now_read = start_pos;
	auto now_pos_in_str = 0;
	auto now_iter = 0;
	//cout << BUF_SIZE << endl;
	while (true)
	{
		count = 0;
		balance = BUF_SIZE - 1 - start_pos;
		now_read = start_pos;
		while (balance > 0 && (count = read(fd_in, buf + now_read, balance)) > 0)
		{
			balance -= count;
			now_read += count;
		}
		//cout << "new read" << endl;
		now_pos_in_str = 0;
		balance = now_read;

		if (old_words_balance > 0)
		{
			for (auto i = 0; i < old_words_balance; i++)
			{
				memcpy(&now_word, buf + now_pos_in_str, byte8);
				//cout << "now_write_word_with_add: " << now_word << endl;
				now_pos_in_str += byte8;
				balance -= byte8;
				doc_words[now_word].push_back(old_doc_id);
				//cout << old_doc_id << endl;
			}
			old_words_balance = 0;
		}
		if (!doc_words.empty())
		{
			//cout << "all good" << endl;
			print_map_in_file(doc_words, fd_out);
			doc_words.clear();
		}
		while (balance > 12)
		{
			//if (balance <= 12)
			//	break;

			memcpy(&doc_id, buf + now_pos_in_str, byte8);
			now_pos_in_str += byte8;
			memcpy(&word_count, buf + now_pos_in_str, byte4);
			now_pos_in_str += byte4;
			//cout << "doc_id count_words: " << doc_id << " " << word_count << endl;

			balance -= byte8 + byte4;

			old_words_balance = word_count;
			old_doc_id = doc_id;

			for (auto i = 0; i < word_count && balance >= 8; i++)
			{
				now_word = 0;
				memcpy(&now_word, buf + now_pos_in_str, byte8);
				//cout << "now_write_word: " << now_word << endl;
				if (doc_words.count(now_word))
				{
					doc_words[now_word].push_back(doc_id);
					//cout << doc_id << endl;
				}
				else
				{
					vector <long long> v{doc_id};
					//cout << doc_id << endl;
					doc_words.insert(doc_words.begin(), pair <int, vector<long long> >(now_word, v));
				}
				now_pos_in_str += byte8;
				balance -= byte8;
				old_words_balance--;
			}
			//cout << "balance is: " << balance << endl;
		}
		if (balance > 0)
		{
			memcpy(buf, buf + now_pos_in_str, balance);
			start_pos = balance;
		}
		if (count <= 0)
			break;
		now_iter++;
	}
	if (!doc_words.empty())
	{
		//cout << "all good" << endl;
		print_map_in_file(doc_words, fd_out);
		doc_words.clear();
	}
	//cout << "all good this is end" << endl;
	//return 0;
	for (auto x : offsets_in_file)
	{
		cout << x.now_pos << " " << x.end_pos << endl;
	}
	close(fd_in);

	vector <long long> words;
	long long min_word = LLONG_MAX;
	for (auto x : offsets_in_file)
	{
		long long word = 0;
		lseek(fd_final, x.now_pos, SEEK_SET);
		read(fd_final, &word, byte8);
		words.push_back(word);
		min_word = min(min_word, word);
	}
	while (true)
	{

		bool flag = 0;
		for (auto x : offsets_in_file)
		{
			if (x.now_pos == x.end_pos)
				continue;
			else
				flag = 1;
			long long word = 0;
			lseek(fd_final, x.now_pos, SEEK_SET);
			read(fd_final, &word, byte8);
			words.push_back(word);
			min_word = min(min_word, word);
		}
		if (!flag)
			break;
	}
	close(fd_out);
	return 0;
}