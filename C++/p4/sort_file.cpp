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
#include <set>
#include <fstream>
#include <limits.h>

#define byte8 8
#define byte4 4
#define mp make_pair
#define f first
#define s second
#define mem_size 32768
#define file_in_name "test"
#define file_out_name "out.bin"
#define result_file "result.bin"

using namespace std;

class offset{
public:
	long long now_pos;
	long long end_pos;
	offset(long long now_pos, long long end_pos) : now_pos(now_pos), end_pos(end_pos) {}
};

class str{
public:
	char *s;
	int now_pos;
	int len;
	bool is_empty;
	str()
	{
		s = NULL;
		now_pos = 0;
		len = 0;
		is_empty = true;
	}
	str(char * s, int now_pos = 0, int len = 0, bool is_empty = false)
	{
		this->s = s;
		this->now_pos = now_pos;
		this->len = len;
		this->is_empty = is_empty;
	}
};

vector <offset> offsets_in_file;
long long now_offset_out_file = 0;
long long unique_words = 0;

void print_map_in_file(map<long long, vector <long long> > m, int fd)
{
	auto beg = m.begin();
	auto end = m.end();
	auto note_size = 0;
	while (beg != end)
	{
		long long word = beg->f;
		vector <long long> docs = beg->s;
		auto it = unique(docs.begin(), docs.end());
		docs.resize(distance(docs.begin(), it));
		sort(docs.begin(), docs.end());

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
	}
	offsets_in_file.push_back(offset(now_offset_out_file, now_offset_out_file + note_size));
	now_offset_out_file += note_size;

}

int main(int argc, char * argv[])
{
	auto fd_in = open(file_in_name, O_RDONLY);
	if (fd_in == -1)
	{
		perror("cannot open input file");
		exit(EXIT_FAILURE);
	}
	auto fd_out = open(file_out_name, O_RDWR | O_CREAT, S_IREAD | S_IWRITE);
	if (fd_out == -1)
	{
		perror("cannot open/create out file");
		exit(EXIT_FAILURE);
	}
	auto fd_final = open(result_file, O_RDWR | O_CREAT, S_IREAD | S_IWRITE);
	if (fd_final == -1)
	{
		perror("cannot open/create result file");
		exit(EXIT_FAILURE);
	}
	auto BUF_SIZE = mem_size / 2;
	char * buf = (char *)malloc(BUF_SIZE);
	
	long long doc_id = 0;
	int word_count = 0;
	long long now_word = 0;

	long long old_doc_id = 0;
	long long old_words_balance = 0;

	map <long long, vector <long long> > doc_words;
	set <long long> myset;

	auto start_pos = 0;
	auto count = 0;
	auto balance = BUF_SIZE - 1 - start_pos;
	auto now_read = start_pos;
	auto now_pos_in_str = 0;
	auto now_iter = 0;
	while (true)
	{
		count = 0;
		balance = BUF_SIZE - 1 - start_pos;
		now_read = start_pos;
		while (balance > 0 && (count = read(fd_in, buf + now_read, balance)) > 0)
		{
			if (count == -1)
			{
				perror("cannot read input file");
				exit(EXIT_FAILURE);
			}
			balance -= count;
			now_read += count;
		}
		now_pos_in_str = 0;
		balance = now_read;
		if (old_words_balance > 0)
		{
			for (auto i = 0; i < old_words_balance; i++)
			{
				memcpy(&now_word, buf + now_pos_in_str, byte8);
				if (!myset.count(now_word))
					myset.insert(now_word);
				now_pos_in_str += byte8;
				balance -= byte8;
				doc_words[now_word].push_back(old_doc_id);
			}
			old_words_balance = 0;
		}
		if (!doc_words.empty())
		{
			print_map_in_file(doc_words, fd_out);
			doc_words.clear();
		}
		while (balance > 12)
		{
			memcpy(&doc_id, buf + now_pos_in_str, byte8);
			now_pos_in_str += byte8;
			memcpy(&word_count, buf + now_pos_in_str, byte4);
			now_pos_in_str += byte4;

			balance -= byte8 + byte4;

			old_words_balance = word_count;
			old_doc_id = doc_id;

			for (auto i = 0; i < word_count && balance >= 8; i++)
			{
				now_word = 0;
				memcpy(&now_word, buf + now_pos_in_str, byte8);
				if (!myset.count(now_word))
					myset.insert(now_word);
				if (doc_words.count(now_word))
					doc_words[now_word].push_back(doc_id);
				else
				{
					vector <long long> v{doc_id};
					doc_words.insert(doc_words.begin(), pair <int, vector<long long> >(now_word, v));
				}
				now_pos_in_str += byte8;
				balance -= byte8;
				old_words_balance--;
			}
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
		print_map_in_file(doc_words, fd_out);
		doc_words.clear();
	}

	unique_words = myset.size();
	long long zero = 0;
	auto beg = myset.begin();
	auto end = myset.end();
	while (beg != end)
	{
		long long ww = (*beg);
		write(fd_final, &ww, sizeof(ww));
		write(fd_final, &zero, sizeof(zero));
		beg++;
	}
	write(fd_final, &zero, sizeof(zero));
	write(fd_final, &zero, sizeof(zero));
	//CLEAR ALL
	myset.clear();
	doc_words.clear();
	free(buf);
	close(fd_in);

	vector <str> parse_dicts;
	int dict_count = offsets_in_file.size();
	long long parse_buf_len = mem_size / dict_count;
	for (auto i = 0; i < dict_count; i++)
	{
		str str_obj;
		str_obj.s = (char *)malloc(parse_buf_len + 10);
		lseek(fd_out, offsets_in_file[i].now_pos, SEEK_SET);
		auto cou_b = read(fd_out, str_obj.s, min(parse_buf_len, offsets_in_file[i].end_pos - offsets_in_file[i].now_pos));
		offsets_in_file[i].now_pos += cou_b;
		str_obj.now_pos = 0;
		str_obj.len = cou_b;
		parse_dicts.push_back(str_obj);
	}
	vector <long long> words;
	vector <int> w_count;
	vector <long long> docs;
	long long start_offset_doc =  (unique_words + 1)* byte8 * 2;
	for (auto i = 0; i < offsets_in_file.size(); i++)
	{
		long long word = 0;
		long long doc = 0;
		int cou = 0;
		memcpy(&word, parse_dicts[i].s + parse_dicts[i].now_pos, byte8);
		parse_dicts[i].now_pos += byte8;
		memcpy(&cou, parse_dicts[i].s + parse_dicts[i].now_pos, byte4);
		parse_dicts[i].now_pos += byte4;
		memcpy(&doc, parse_dicts[i].s + parse_dicts[i].now_pos, byte8);
		parse_dicts[i].now_pos += byte8;
		words.push_back(word);
		w_count.push_back(cou);
		docs.push_back(doc);
	}

	long long prev_min_word = -1;
	set <long long> d;
	vector <long long> off;
	vector <int> w_c;
	int cou = 1;

	while(true)
	{

		long long min_doc = LLONG_MAX;
		long long min_word = LLONG_MAX;
		int pos_min_doc = -1;
		bool flag = false;
		for (auto i = 0; i < words.size(); i++)
		{
			if (w_count[i] == 0)
				continue;
			else
				flag = true;
			if (words[i] < min_word)
			{
				min_word = words[i];
				min_doc = docs[i];
				pos_min_doc = i;
			}
			else if (words[i] == min_word && docs[i] < min_doc)
			{
				min_doc = docs[i];
				pos_min_doc = i;
			}
		}
		if (!flag)
			break;
		if (prev_min_word != min_word)
		{
			int zero = 0;
			write(fd_final, &zero, sizeof(int));
			write(fd_final, &min_doc, sizeof(long long));
			off.push_back(start_offset_doc);
			if (prev_min_word != -1)
				w_c.push_back(cou);
			start_offset_doc += sizeof(int) + sizeof(long long);
			prev_min_word = min_word;
			cou = 1;
		}
		else if (prev_min_word == min_word)
		{
			write(fd_final, &min_doc, sizeof(min_doc));
			start_offset_doc += sizeof(min_doc);
			cou++;
		}
		w_count[pos_min_doc]--;
		for (auto i = 0; i < dict_count; i++)
		{
			if (parse_dicts[i].len - parse_dicts[i].now_pos < 20 && offsets_in_file[i].end_pos != offsets_in_file[i].now_pos)
			{
				lseek(fd_out, offsets_in_file[i].now_pos, SEEK_SET);
				memcpy(parse_dicts[i].s, parse_dicts[i].s + parse_dicts[i].now_pos, int(parse_dicts[i].len - parse_dicts[i].now_pos));
				parse_dicts[i].now_pos = parse_dicts[i].len - parse_dicts[i].now_pos;
				auto cou_b = read(fd_out, parse_dicts[i].s + parse_dicts[i].now_pos, min(parse_dicts[i].len - parse_dicts[i].now_pos, int(offsets_in_file[i].end_pos - offsets_in_file[i].now_pos)));
				parse_dicts[i].len = parse_dicts[i].now_pos + cou_b;
				parse_dicts[i].now_pos = 0;
				offsets_in_file[i].now_pos += cou_b;
			}
		}

		if (w_count[pos_min_doc] > 0)
		{
			memset(&(docs[pos_min_doc]), 0, sizeof(docs[pos_min_doc]));
			memcpy(&(docs[pos_min_doc]), parse_dicts[pos_min_doc].s + parse_dicts[pos_min_doc].now_pos, sizeof(docs[pos_min_doc]));
			parse_dicts[pos_min_doc].now_pos += sizeof(docs[pos_min_doc]);
		}
		else if (!w_count[pos_min_doc] && parse_dicts[pos_min_doc].now_pos < parse_dicts[pos_min_doc].len)
		{

			memcpy(&(words[pos_min_doc]), parse_dicts[pos_min_doc].s + parse_dicts[pos_min_doc].now_pos, sizeof(words[pos_min_doc]));
			parse_dicts[pos_min_doc].now_pos += sizeof(words[pos_min_doc]);
			memcpy(&(w_count[pos_min_doc]), parse_dicts[pos_min_doc].s + parse_dicts[pos_min_doc].now_pos, sizeof(w_count[pos_min_doc]));
			parse_dicts[pos_min_doc].now_pos += sizeof(w_count[pos_min_doc]);
			memcpy(&(docs[pos_min_doc]), parse_dicts[pos_min_doc].s + parse_dicts[pos_min_doc].now_pos, sizeof(docs[pos_min_doc]));
			parse_dicts[pos_min_doc].now_pos += sizeof(docs[pos_min_doc]);
		}


	}
	w_c.push_back(cou);

	lseek(fd_final, 0, SEEK_SET);

	for (auto i = 0; i < off.size(); i++)
	{
		lseek(fd_final, byte8, SEEK_CUR);
		write(fd_final, &(off[i]), sizeof(off[i]));
	}
	for (auto i = 0; i < w_c.size(); i++)
	{
		lseek(fd_final, off[i], SEEK_SET);
		write(fd_final, &(w_c[i]), sizeof(int));
	}

	for (auto i = 0; i < dict_count; i++)
		free(parse_dicts[i].s);	
	close(fd_out);
	close(fd_final);
	return 0;
}