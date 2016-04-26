#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <algorithm>
#include <fcntl.h>
#include <map>
#include <set>
#include <string>
#include <unistd.h>
#include <cstring>
#include <sys/types.h> 
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define f first
#define s second
#define mp make_pair

using namespace std;

void hd1(int sig)
{
	sleep(10);
	return ;
}

struct comand
{
	char * comand_name = nullptr;
	char ** args = nullptr;
	int args_count;
	string in = "";
	string out = "";
	comand(string s, vector <string> v, string s_in, string s_out)
	{
		in = s_in;
		out = s_out;
		//cout « s « endl;
		//cout « v[0] « endl;
		args_count = v.size();

		comand_name = (char *)malloc(sizeof(char) * (s.size() + 1));
		memcpy(comand_name, s.c_str(), s.size());
		comand_name[s.size()] = '\0';
		//cout « "com name is:" « comand_name « endl;
		this->args = (char **)malloc(sizeof(char *) * (v.size() + 1));

		for (auto i = 0; i < v.size(); i++)
		{
			args[i] = (char *)malloc(sizeof(char) * (v[i].size() + 1));
			memcpy(args[i], v[i].c_str(), v[i].size());
			args[i][v[i].size()] = '\0';
		}
		this->args[v.size()] = NULL;
	}
	comand(const comand & B)
	{
		comand_name = (char *)malloc(sizeof(char) * (strlen(B.comand_name) + 1));
		memcpy(comand_name, B.comand_name, strlen(B.comand_name));
		comand_name[strlen(B.comand_name)] = '\0';

		this-> args = (char **)malloc(sizeof(char *) * (B.args_count + 1));
		memcpy(args, B.args, B.args_count);
		args[B.args_count] = nullptr;
		for (auto i = 0; i < B.args_count; i++)
		{
			args[i] = (char *) malloc(sizeof(char) * (strlen(B.args[i]) + 1));
			memcpy(args[i], B.args[i], strlen(B.args[i]));
			args[i][strlen(B.args[i])] = '\0';
		}
		this-> args_count = B.args_count;
		this-> in = B.in;
		this-> out = B.out;
	} 
	~comand()
	{
		free(comand_name);
		for (auto i = 0; i < args_count; i++)
		free(args[i]);
		free(args);
	}
};

struct expression{
	vector <comand> comands;
	vector <string> op;

	expression(vector <comand> c, vector <string> o)
	{
		this->comands = c;
		this->op = o;
	}
};

void print_expression(expression expr)
{
	vector<comand> c = expr.comands;
	vector <string> op = expr.op;
	cout << "Comands count is: " << c.size() << endl;
	for (auto i = 0; i < c.size(); i++)
	{
		cout << "Comand name is: " << c[i].comand_name << " ATTRS ARE: ";
		for (auto j = 0; j < c[i].args_count; j++)
		{
			cout << c[i].args[j] << " ";
		}
		cout << "Input Output: " << c[i].in << " " << c[i].out << endl; 
	} 

	cout << endl;
	cout << "Operations count is: " << op.size() << endl;
	cout << "Operations are: ";
	for (auto i = 0; i < op.size(); i++)
	{
		cout << op[i] << " ";
	}
	cout<<endl;
	return ;
}

struct proc_attr{
	int pid;
	int st;
	proc_attr(int p, int s): pid(p), st(s) {}
};

comand parse_comand(string s)
{
string comand_name = "";
string now_in = "";
string now_out = "";
vector <string> args;
auto pos = 0;
string now_s = "";

while (pos < s.size() && s[pos] == ' ')
pos++;

while (pos < s.size() && s[pos] != ' ' && s[pos] != '<' && s[pos] != '>')
{
comand_name += s[pos];
pos++;
}
args.push_back(comand_name);
bool flag_in = false;
bool flag_out = false;
string in = "";
string out = "";
now_s = "";
while (pos < s.size())
{
if (s[pos] == '>')
{
if (now_s.size())
{
args.push_back(now_s);
now_s = "";
}
flag_out = true;
}
else if (s[pos] == '<')
{
if (now_s.size())
{
args.push_back(now_s);
now_s = "";
}
flag_in = true;
}
else if (s[pos] == ' ' || s[pos] == '\n')
{
if (now_s.size())
{
if (flag_in)
{
flag_in = false;
in = now_s;
}
else if (flag_out)
{
flag_out = false;
out = now_s;
}
else 
args.push_back(now_s);
}
now_s = "";
}
else
now_s += s[pos];
pos++;
}
if (now_s.size())
{
if (flag_in)
{
flag_in = false;
in = now_s;
}
else if (flag_out)
{
flag_out =
10:11:19	
false;
out = now_s;
}
else 
args.push_back(now_s);
}
//comand cc = comand(comand_name, args, in, out);
//cout « cc.comand_name « " " « cc.args[0] « endl;
return comand(comand_name, args, in, out);
}

expression parse_expression(string s)
{
auto pos = 0;
auto prev_pos = 0;
vector <comand> v;
vector <string> op;
string now_s = "";
while (pos < s.size())
{
if ((s[pos] == '&' && s[pos + 1] == '&') || (s[pos] == '|' && s[pos + 1] == '|'))
{
v.push_back(parse_comand(now_s));
now_s = "";
string ss = "";
ss += s[pos];
ss += s[pos + 1];
op.push_back(ss);
pos++;
}
else if (s[pos] == '|' && s[pos + 1] != '|')
{
v.push_back(parse_comand(now_s));
string ss = "";
ss += s[pos];
op.push_back(ss);
}
else
{
now_s += s[pos];
}
pos++;
}
//cout « "NOW STRING: " « now_s « endl;

if (now_s.size())
{
//comand cc = parse_comand(now_s);
//cout « "I here man: " « cc.comand_name « " " « cc.args[0] « endl;
v.push_back(parse_comand(now_s));
}
/*cout « v.size() « endl;
cout « op.size() « endl; if (!inf)
cout « v[0].comand_name « endl;
cout « v[0].args_count « endl;
cout « v[0].args[0] « endl;
cout « "lala" « endl;*/
return expression(v, op);
}

proc_attr exe_comand(comand com, bool is_or)
{
//cout « "I here man" « endl;
if (!fork())
{
//cout « "LAMBDA" « endl;
if (com.in.size())
{
auto fd_in = open(com.in.c_str(), O_RDONLY);
dup2(fd_in, 0);
}
if (com.out.size())
{
auto fd_out = open(com.out.c_str(), O_WRONLY | O_CREAT | O_APPEND | O_TRUNC, S_IREAD | S_IWRITE);
dup2(fd_out, 1);
}
execvp(com.comand_name, com.args);
exit(1);
}
int st = 0;
int pid = wait(&st);
//cout « "LALALALLA" « endl;
cerr « "Process " « pid « " exited: " « st « endl;
return proc_attr(pid, st);
}

int main(int argc, char * argv[])
{
string s;
setbuf(stdout, NULL);
setbuf(stderr, NULL);
while (getline(cin, s))
{
//cout « s « endl;
expression now_expression = parse_expression(s);
//print_expression(now_expression);
//cout « "all good" « endl « endl « endl;

//cout « "all good" « endl;
/*cout « now_comand.comand_name « " ";
for (auto i = 0; i < now_comand.args_count; i++)
{
cout « now_comand.args[i] « " ";
}
cout « "INPUT OUTPUT"« now_comand.in « " " « now_comand.out;
cout « endl;*/
if (!fork())
{
auto next_op = 0;
if (next_op < now_expression.op.size() && now_expression.op[next_op] == '|')
{
auto info = exe_comand(now_expression.comands[0]);
}

//cout « "PROC END" « endl;
for (auto i = 0; i < now_expression.op.size(); i++)
{
next_op++;

//cout « "yes op" « endl;
if (next_op < now_expression.op.size() && now_expression.op[next_op] == "|")
{
if (!info.st)
{

}
}
if (now_expression.op[i] == "&&")
{
if (!info.st)
{
info = exe_comand(now_expression.comands[i + 1]);
}
}
else if (now_expression.op[i] == "||")
{
if (info.st)
{
info = exe_comand(now_expression.comands[i + 1]);
}
}
}
/*if (now_comand.in.size())
{
auto fd_in = open(now_comand.in.c_str(), O_RDONLY);
dup2(fd_in, 0);
}
if (now_comand.out.size())
{
auto fd_out = open(now_comand.out.c_str(), O_WRONLY | O_CREAT | O_APPEND | O_TRUNC, S_IREAD | S_IWRITE);
dup2(fd_out, 1);
}
execvp(now_comand.comand_name, now_comand.args);*/
exit(0);
}
int st = 0;
int pid = wait(&st);
//cerr « "Process " « pid « " exited: " « st « endl;
}
return 0;
}