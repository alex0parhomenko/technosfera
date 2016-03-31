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

#define byte8 8
#define byte4 4
#define simple_const 77
#define make_pair mp
#define first f
#define second s

auto file_size = 0;
auto memory_size = 0;
auto max_use_memory = 0;
auto now_file_num = 0;

int main(int argc, char * argv[])
{
	std::string f_s(argv[1]), m_s(argv[2]);
	std::stringstream f_size(f_s), mem_s(m_s);
	std::stringstream ss;
	max_use_memory = memory_size / 2;
	std::vector <std::string> file_names;
	char buf[max_use_memory];
	char buf_8[byte8 + 1];
	char buf_4[byte4 + 1];
	f_size >> file_size;
	mem_s >> memory_size;
	std::map <int, vector <int> > d;
	std::set<int> words;
	auto fd = open(argv[3]);
	auto num_it = 0;
	auto now_bytes = 0;
	auto now_pos_in_file = 0;
	auto cou = 0;
	auto fd_now = 0;
	auto now_pos_in_str;
	auto is_eof = false;
	while (true)
	{
		auto cou_blocks = 0;
		while (true)
		{
			cou = read(fd, buf_8, byte8);
			if (cou <= 0)
			{
				is_eof = true;
				break;
			}
			cou = read(fd, buf_4, byte4);
			auto file_count = 0;
			sscanf(buf_4, "%d", file_count);
			fseek(fd, -12, SEEK_CUR);
			if (now_bytes + byte8 + byte4 + file_count * byte8 + simple_const > max_use_memory)
				break;	
			else
			{
				cou = read(fd, buf + now_bytes, byte8 + byte4 + file_count * 8);
				now_bytes += byte8 + byte4 + file_count * 8;
			}
			cou_blocks++;
		}
		buf[now_bytes] = '\0';
		std::vector <pair<int, int>> word_doc;
		auto now_pos_in_str = 0;
		for (auto i = 0; i < cou_blocks; i++)
		{
			auto doc_id = 0;
			auto cou_f = 0;
			sscanf(buf, "%d", &doc_id);
			sscanf(buf + 4, "%d", &cou_f);
			now_pos_in_str += 8;
			for (auto j = 0; j < cou_f; j++)
			{
				auto word = 0;
				sscanf(buf, "%d", &word);
				word_doc.push_back(mp(word, doc_id));
			}
		}
		for (auto p in word_doc)
		{
			d[p[0]].push_back(p[1]);
			words.insert(word);
		}
		memset(buf, 0, sizeof(buf));
		ss << now_file_num;
		file_names.push_back(ss.str());
		fd_now = open(file_names[file_names.size() - 1], O_CREAT|O_WRONLY);
		now_file_num++;
	}

	close(fd);
	return 0;
}