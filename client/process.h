
#ifndef __PROCESS_H
#define __PROCESS_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>

#ifdef WIN32

#include <windows.h>
#include <io.h>

#else

#include <unistd.h>
#include <spawn.h>
#include <sys/wait.h>

#endif

#define DUPLICATE_STRING(S) strcpy((char*) malloc(sizeof(char) * strlen(S)), S)

#include <map>
#include <vector>

using namespace std;

class Process {
public:

    Process(string command);
    ~Process();

    void set_directory(string pwd);
    void set_environment(string key, string value);

    void copy_environment();

    bool start();
    bool stop();

    FILE* get_input();
    FILE* get_output();

    int get_handle();

    bool is_alive();

private:

    void cleanup();

    FILE* p_stdout;
    FILE* p_stdin;

    int out[2];
    int in[2];

    char** arguments;
    char* program;

    string directory;
    map<string, string> env;

#ifdef WIN32
	// http://msdn.microsoft.com/en-us/library/windows/desktop/ms682499%28v=vs.85%29.aspx
	// http://msdn.microsoft.com/en-us/library/dye30d82%28VS.80%29.aspx
	HANDLE handle_IN_Rd;
	HANDLE handle_IN_Wr;
	HANDLE handle_OUT_Rd;
	HANDLE handle_OUT_Wr;
	PROCESS_INFORMATION piProcInfo;
#else
	int pid;

    posix_spawn_file_actions_t action;
#endif

};

#endif
