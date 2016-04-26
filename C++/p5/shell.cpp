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
#include <set>
#define f first
#define s second
#define mp make_pair

using namespace std;


struct proc_type{
    int pid;
    int st;
    int type;
    proc_type(int a, int b, int c): pid(a), st(b), type(c) {}
};

set <int> all_proc;
set <int> back_proc;

void hd1(int sig)
{
    signal(SIGINT, hd1);
    auto beg = all_proc.begin();
    auto end = all_proc.end();
    while (beg != end)
    {
        kill(*beg, sig);
        beg++;
    }
}


struct comand
{
    char * comand_name = nullptr;
    char ** args = nullptr;
    int args_count;
    bool is_background;
    string in = "";
    string out = "";
    comand(string s, vector <string> v, string s_in, string s_out, bool is_back)
    {
        this->is_background = is_back;
        in = s_in;
        out = s_out;
        args_count = v.size();

        comand_name = (char *)malloc(sizeof(char) * (s.size() + 1));
        memcpy(comand_name, s.c_str(), s.size());
        comand_name[s.size()] = '\0';
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
        this-> out  = B.out;
        this -> is_background = B.is_background;
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
    bool is_background;
    expression(vector <comand> c, vector <string> o, bool flag)
    {
        this->is_background = flag;
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
        cout << "IS background: " << c[i].is_background << endl;
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
    bool is_background = false;
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
            flag_out = false;
            out = now_s;
        }
        else 
            args.push_back(now_s);
    }
    if (args[args.size() - 1] == "&")
    {
        is_background = true;
        args.pop_back();
    }
    return comand(comand_name, args, in, out, is_background);
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
            now_s = "";
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
    if (now_s.size())
        v.push_back(parse_comand(now_s));
    bool flac = v[v.size() - 1].is_background;
    return expression(v, op, flac);
}


proc_attr exe_comand(comand com, int fd_in, int fd_out, bool is_wait, bool is_index)
{
    int now_pid = 0;
    if (!(now_pid = fork()))
    {
        if (fd_in != -1)
        {
            dup2(fd_in, 0);
        }
        else if (com.in.size())
        {
            auto fd_in = open(com.in.c_str(), O_RDONLY);
            dup2(fd_in, 0);
        }
        if (fd_out != -1)
        {
            dup2(fd_out, 1);
        }
        else if (com.out.size())
        {
            auto fd_out = open(com.out.c_str(), O_WRONLY | O_CREAT | O_APPEND | O_TRUNC, S_IREAD | S_IWRITE);
            dup2(fd_out, 1);
        }
        execvp(com.comand_name, com.args);
        exit(0);
    }
    if (is_wait)
    {
        int st = 0;
        int pid = 0;
        if (is_index)
            all_proc.insert(now_pid);
        while (waitpid(now_pid, &st, 0) >= 0) {}
        if (is_index)
            all_proc.erase(now_pid);
        cerr << "Process " << now_pid <<  " exited: " << WEXITSTATUS(st) << endl;
        return proc_attr(pid, st);
    }
    return proc_attr(-1, -1);
}

void exe_expr(expression now_expression, bool is_index)
{
    int background_procceses = 0;
    auto next_op = 0;

    int fd_pipe[40] = {0};
    auto pos_fd_pipe = 0;
    int parallel_proccess_count = 0;
    auto info = proc_attr(-1, -1);
    if (next_op < now_expression.op.size() && now_expression.op[next_op] == "|")
    {
        pipe(fd_pipe + pos_fd_pipe);
        info = exe_comand(now_expression.comands[0], -1, fd_pipe[pos_fd_pipe + 1], false, is_index);
        close(fd_pipe[pos_fd_pipe + 1]);
        pos_fd_pipe += 2;
        parallel_proccess_count += 1;   
    }
    else
    {
        info = exe_comand(now_expression.comands[0], -1, -1, true, is_index);
    }
    for (auto i = 0; i < now_expression.op.size(); i++)
    {
        next_op++;
        if (now_expression.op[i] == "&&")
        {
            if (!info.st)
            {
                if (next_op < now_expression.op.size() && now_expression.op[next_op] == "|")
                {
                    pipe(fd_pipe + pos_fd_pipe);
                    info = exe_comand(now_expression.comands[i + 1], -1, fd_pipe[pos_fd_pipe + 1], false, is_index);
                    close(fd_pipe[pos_fd_pipe + 1]);
                    parallel_proccess_count++;
                    pos_fd_pipe += 2;
                }
                else
                {
                    parallel_proccess_count = 0;
                    info = exe_comand(now_expression.comands[i + 1], -1, -1, true, is_index);
                }
            }
        }
        else if (now_expression.op[i] == "||")
        {
            if (info.st)
            {
                if (next_op < now_expression.op.size() && now_expression.op[next_op] == "|")
                {
                    pipe(fd_pipe + pos_fd_pipe);
                    info = exe_comand(now_expression.comands[i + 1], -1, fd_pipe[pos_fd_pipe + 1], false, is_index);
                    close(fd_pipe[pos_fd_pipe + 1]);
                    parallel_proccess_count++;
                    pos_fd_pipe += 2;
                }
                else
                {
                    parallel_proccess_count = 0;
                    info = exe_comand(now_expression.comands[i + 1], -1, -1, true, is_index);
                }
            }
        }
        else if (now_expression.op[i] == "|")
        {
            if (parallel_proccess_count)
            {
                if (next_op < now_expression.op.size() && now_expression.op[next_op] == "|")
                {
                    pipe(fd_pipe + pos_fd_pipe);
                    info = exe_comand(now_expression.comands[i + 1], fd_pipe[pos_fd_pipe - 2], fd_pipe[pos_fd_pipe + 1], false, is_index);
                    close(fd_pipe[pos_fd_pipe + 1]);
                    close(fd_pipe[pos_fd_pipe - 2]);
                    parallel_proccess_count++;
                    pos_fd_pipe += 2;
                }
                else
                {
                    info = exe_comand(now_expression.comands[i + 1], fd_pipe[pos_fd_pipe - 2], -1, true, is_index);
                    close(fd_pipe[pos_fd_pipe - 2]);
                    parallel_proccess_count = 0;
                    pos_fd_pipe += 2;
                }
            }
        }

    }
    for (auto i = 0; i < 40; i++)
        if (fd_pipe[i])
            close(fd_pipe[i]);
}


int main(int argc, char * argv[])
{
    signal(SIGINT, hd1);
    string s;
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);
    while (getline(cin, s))
    {
        if (!s.size())
            continue;
        expression now_expression = parse_expression(s);
        if (now_expression.is_background)
        {
            auto now_pid=0;
            if (!(now_pid = fork()))
            {
                exe_expr(now_expression, false);
                exit(0);
            }
        }
        else
            exe_expr(now_expression, true);

    }
    int st = 0;
    while (wait(&st) != -1){}
    return 0;
}