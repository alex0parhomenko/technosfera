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


struct proc_type{
    int pid;
    int st;
    int type;
    proc_type(int a, int b, int c): pid(a), st(b), type(c) {}
};

vector <int> all_proc;


void hd1(int sig)
{
    for (auto i = 0; i < all_proc.size(); i++)
    {
        kill(SIGINT, all_proc[i]);
    }
    return ;
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
        //cout << s << endl;
        //cout << v[0] << endl;
        args_count = v.size();

        comand_name = (char *)malloc(sizeof(char) * (s.size() + 1));
        memcpy(comand_name, s.c_str(), s.size());
        comand_name[s.size()] = '\0';
        //cout <<  "com name is:" << comand_name << endl;
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
    //comand cc =  comand(comand_name, args, in, out);
    //cout << cc.comand_name << " " << cc.args[0] << endl;
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
        /*else if (s[pos] == '&' && (pos + 1 >= s.size() || s[pos + 1] != '&' ))
        {
            v.push_back(parse_comand(now_s));
            now_s = "";
            string ss = "";
            ss += s[pos];
            op.push_back(ss);
        }*/
        else
        {
            now_s += s[pos];
        }
        pos++;
    }
    //cout << "NOW STRING: " << now_s << endl;

    if (now_s.size())
    {
        //comand cc = parse_comand(now_s);
        //cout << "I here man: " << cc.comand_name << " " << cc.args[0] << endl;
        v.push_back(parse_comand(now_s));
    }
    bool flac = v[v.size() - 1].is_background;
    /*cout << v.size() << endl;
    cout << op.size() << endl;                  if (!inf)
    cout << v[0].comand_name << endl;
    cout << v[0].args_count << endl;
    cout << v[0].args[0] << endl;
    cout << "lala" << endl;*/
    return expression(v, op, flac);
}


proc_attr exe_comand(comand com, int fd_in, int fd_out, bool is_wait)
{
    int now_pid = 0;
    if (!(now_pid = fork()))
    {
        //cout << "secproc\n";
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
    //all_proc.push_back(proc_type(now_pid, -1, com.is_background));
    /*int st = 0;
    int pid = wait(&st);
    //cout << "LALALALLA" << endl;
    cerr << "Process " << pid <<  " exited: " << st << endl;
    return proc_attr(pid, st);*/
    if (is_wait)
    {
        int st = 0;
        int pid = 0;
        while (waitpid(now_pid, &st, 0) >= 0) {}
        cerr << "Process " << now_pid <<  " exited: " << WEXITSTATUS(st) << endl;
        return proc_attr(pid, st);
    }
    return proc_attr(-1, -1);
}

void exe_expr(expression now_expression)
{
    //print_expression(now_expression);
    int background_procceses = 0;
    auto next_op = 0;

    //cout << "PROC END" << endl;
    int fd_pipe[40] = {0};
    auto pos_fd_pipe = 0;
    int parallel_proccess_count = 0;
    auto info = proc_attr(-1, -1);
    if (next_op < now_expression.op.size() && now_expression.op[next_op] == "|")
    {
        //cout << "I HERE MAN \n";
        pipe(fd_pipe + pos_fd_pipe);
        info = exe_comand(now_expression.comands[0], -1, fd_pipe[pos_fd_pipe + 1], false);
        close(fd_pipe[pos_fd_pipe + 1]);
        pos_fd_pipe += 2;
        parallel_proccess_count += 1;   
    }
    else
    {
        info = exe_comand(now_expression.comands[0], -1, -1, true);
    }
    for (auto i = 0; i < now_expression.op.size(); i++)
    {
        //cout << "BAD\n";
        next_op++;
        if (now_expression.op[i] == "&&")
        {
            //cout << "&&\n";
            if (!info.st)
            {
                if (next_op < now_expression.op.size() && now_expression.op[next_op] == "|")
                {
                    pipe(fd_pipe + pos_fd_pipe);
                    info = exe_comand(now_expression.comands[i + 1], -1, fd_pipe[pos_fd_pipe + 1], false);
                    close(fd_pipe[pos_fd_pipe + 1]);
                    parallel_proccess_count++;
                    pos_fd_pipe += 2;
                }
                else
                {
                    parallel_proccess_count = 0;
                    info = exe_comand(now_expression.comands[i + 1], -1, -1, true);
                }
            }
        }
        else if (now_expression.op[i] == "||")
        {
            //cout << "||\n";
            if (info.st)
            {
                if (next_op < now_expression.op.size() && now_expression.op[next_op] == "|")
                {
                    pipe(fd_pipe + pos_fd_pipe);
                    info = exe_comand(now_expression.comands[i + 1], -1, fd_pipe[pos_fd_pipe + 1], false);
                    close(fd_pipe[pos_fd_pipe + 1]);
                    parallel_proccess_count++;
                    pos_fd_pipe += 2;
                }
                else
                {
                    parallel_proccess_count = 0;
                    info = exe_comand(now_expression.comands[i + 1], -1, -1, true);
                }
            }
        }
        else if (now_expression.op[i] == "|")
        {
            //cout << "|\n";
            if (parallel_proccess_count)
            {
                if (next_op < now_expression.op.size() && now_expression.op[next_op] == "|")
                {
                    pipe(fd_pipe + pos_fd_pipe);
                    info = exe_comand(now_expression.comands[i + 1], fd_pipe[pos_fd_pipe - 2], fd_pipe[pos_fd_pipe + 1], false);
                    close(fd_pipe[pos_fd_pipe + 1]);
                    close(fd_pipe[pos_fd_pipe - 2]);
                    parallel_proccess_count++;
                    pos_fd_pipe += 2;
                }
                else
                {
                    //cout << "I NER\n";
                    //info = exe_comand(now_expression.comands[i + 1], -1, -1, true);
                    info = exe_comand(now_expression.comands[i + 1], fd_pipe[pos_fd_pipe - 2], -1, true);
                    close(fd_pipe[pos_fd_pipe - 2]);
                    parallel_proccess_count = 0;
                    pos_fd_pipe += 2;
                }
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
    for (auto i = 0; i < 40; i++)
    {
        if (fd_pipe[i] > 1)
        {
            close(fd_pipe[i]);
        }
    }
}


int main(int argc, char * argv[])
{
    //signal(SIGCHLD, hdchld);
    signal(SIGINT, hd1);
    string s;
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);
    while (getline(cin, s))
    {
        //cout << s << endl;
        if (!s.size())
            continue;
        expression now_expression = parse_expression(s);
        if (now_expression.is_background)
        {
            //cout << "I HERE\n";
            auto now_pid=0;
            if (!(now_pid = fork()))
            {
                exe_expr(now_expression);
                exit(0);
            }
            all_proc.push_back(now_pid);
        }
        else
            exe_expr(now_expression);
        //print_expression(now_expression);
        //continue;
        //cout << "all good" << endl << endl << endl;

        //cout << "all good" << endl;
        /*cout << now_comand.comand_name << " ";
        for (auto i = 0; i < now_comand.args_count; i++)
        {
            cout << now_comand.args[i] << " ";
        }
        cout << "INPUT OUTPUT"<< now_comand.in << " " << now_comand.out;
        cout << endl;*/
        //int now_pid = 0;
        //if (!(now_pid = fork()))
        //{
            
            //cout << "AZAZA\n";
            //cout << bool(cin) << endl;
            //cout << "THE END\n";
            //exit(0);
        //}
        //all_proc.push_back(proc_type(now_pid, -1, 0));
        //int st = 0;
        //int pid = wait(&st);
    }
    for (auto i = 0; i < all_proc.size(); i++)
    {
        int st = 0;
        while (waitpid(all_proc[i], &st, 0) == -1) {}
    }
    return 0;
}