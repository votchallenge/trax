
#ifndef __PROCESS_H
#define __PROCESS_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>

#if defined(__OS2__) || defined(__WINDOWS__) || defined(WIN32) || defined(WIN64) || defined(_MSC_VER)

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

#include "threads.h"

using namespace std;

class Process : public Synchronized {
public:

    Process(string command, bool explicit_mode = false);
    ~Process();

    void set_directory(string pwd);
    void set_environment(string key, string value);

    void copy_environment();

    bool start();
    bool stop();
    bool kill();

    int get_input();
    int get_output();
    int get_error();

    int get_handle();

    bool is_alive(int *status = NULL);

    // Explicit streams
    bool is_explicit();

private:

    void cleanup();

    int p_stdout;
    int p_stdin;
    int p_stderr;
    int p_eout;
    int p_ein;
    int p_eerr;

    int out[2];
    int in[2];
    int err[2];

    char** arguments;
    char* program;

    string directory;
    map<string, string> env;

    bool running;

    int explicit_mode;
    int exit_status;

#ifdef WIN32
	// http://msdn.microsoft.com/en-us/library/windows/desktop/ms682499%28v=vs.85%29.aspx
	// http://msdn.microsoft.com/en-us/library/dye30d82%28VS.80%29.aspx
	HANDLE handle_IN_Rd;
	HANDLE handle_IN_Wr;
	HANDLE handle_OUT_Rd;
	HANDLE handle_OUT_Wr;
	HANDLE handle_ERR_Rd;
	HANDLE handle_ERR_Wr;

	HANDLE handle_explicit_IN_Rd;
	HANDLE handle_explicit_IN_Wr;
	HANDLE handle_explicit_OUT_Rd;
	HANDLE handle_explicit_OUT_Wr;
	HANDLE handle_explicit_ERR_Rd;
	HANDLE handle_explicit_ERR_Wr;

	PROCESS_INFORMATION piProcInfo;
#else
	int pid;

    posix_spawn_file_actions_t action;

    bool action_initialized;
#endif

};

#endif
