
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <stdexcept>
#include <iostream>

#include "process.h"

#ifdef WIN32

#include <sstream>

#include <fcntl.h>

#else

extern char **environ;

const string __getcwd()
{
    size_t buf_size = 1024;
    char* buf = NULL;
    char* r_buf;

    do {
      buf = static_cast<char*>(realloc(buf, buf_size));
      r_buf = getcwd(buf, buf_size);
      if (!r_buf) {
        if (errno == ERANGE) {
          buf_size *= 2;
        } else {
          free(buf);
          throw std::runtime_error("Unable to obtain current directory"); 
        }
      }
    } while (!r_buf);

    string str(buf);
    free(buf);
    return str;
}

#endif

int __next_token(const char* str, int start, char* buffer, int len) 
{
    int i;
    int s = -1;
    int e = -1;
    short quotes = 0;

    for (i = start; str[i] != '\0' ; i++) 
    {
        if (s < 0 && str[i] != ' ')
        {
            if (str[i] == '"') 
            {
                quotes = 1;
                s = i+1;
            }
            else s = i;
            continue;
        }
        if (s >= 0 && ((!quotes && str[i] == ' ') || (quotes && str[i] == '"' && (i == start || str[i-1] != '\''))))
        {
            e = i;
            break;
        }
    }

    buffer[0] = '\0';

    if (s < 0) return -1;

    if (e < 0 && !quotes) e = i;
    if (e < 0 && quotes) return -1;

    if (e - s > len - 1) return -1;


    memcpy(buffer, &(str[s]), e - s);

    buffer[e - s] = '\0';

    return str[e] == '\0' ? e : (quotes ? e+2 : e+1);
}

char** parse_command(const char* command) {

    int length = (int)strlen(command);
    char* buffer = new char[length];
    char** tokens = new char*[length];
    int position = 0;
    int i = 0;

    while (1) {
        
        position = __next_token(command, position, buffer, 1024); 

        if (position < 0) {
            for (int j = 0; j < i; j++) delete[] tokens[j];
            delete [] tokens;
            delete [] buffer;
            return NULL;
        }

        char* tmp = (char*) malloc(sizeof(char) * (strlen(buffer) + 1));

        strcpy(tmp, buffer);
        tokens[i++] = tmp;

        if (position >= length)
            break;

    }

    if (!i) {
        delete [] tokens;
        delete [] buffer;
        return NULL;
    }

    char** result = new char*[i+1];
    for (int j = 0; j < i; j++) result[j] = tokens[j];
    result[i] = NULL;
    delete [] tokens;
    delete [] buffer;
    return result;

}


Process::Process(string command) {

#ifdef WIN32
	piProcInfo.hProcess = 0;
#else
	pid = 0;
#endif

    char** tokens = parse_command(command.c_str());

    if (!tokens[0])
        throw std::runtime_error("Unable to parse command string");

    program = tokens[0];
    arguments = tokens;

}

Process::~Process() {

    int i = 0;

    while (arguments[i]) {
        free(arguments[i]);
        i++;
    }

    delete [] arguments;

    stop();


}

bool Process::start() {

#ifdef WIN32

	handle_IN_Rd = NULL;
	handle_IN_Wr = NULL;
	handle_OUT_Rd = NULL;
	handle_OUT_Wr = NULL;

	p_stdin = NULL;
	p_stdout = NULL;

	SECURITY_ATTRIBUTES saAttr; 

	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
	saAttr.bInheritHandle = TRUE; 
	saAttr.lpSecurityDescriptor = NULL; 

	// Create a pipe for the child process's STDOUT. 
 
   if ( ! CreatePipe(&handle_OUT_Rd, &handle_OUT_Wr, &saAttr, 0) ) 
      return false; 

	// Ensure the read handle to the pipe for STDOUT is not inherited.

   if ( ! SetHandleInformation(handle_OUT_Rd, HANDLE_FLAG_INHERIT, 0) )
      return false; 

	// Create a pipe for the child process's STDIN. 
 
   if (! CreatePipe(&handle_IN_Rd, &handle_IN_Wr, &saAttr, 0)) 
      return false; 

	// Ensure the write handle to the pipe for STDIN is not inherited. 
 
   if ( ! SetHandleInformation(handle_IN_Wr, HANDLE_FLAG_INHERIT, 0) )
      return false; 
 
	STARTUPINFO siStartInfo;
	BOOL bSuccess = FALSE; 
 
	// Set up members of the PROCESS_INFORMATION structure. 
 
	ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );
 
	// Set up members of the STARTUPINFO structure. 
	// This structure specifies the STDIN and STDOUT handles for redirection.
 
	ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
	siStartInfo.cb = sizeof(STARTUPINFO); 
	siStartInfo.hStdError = handle_OUT_Wr;
	siStartInfo.hStdOutput = handle_OUT_Wr;
	siStartInfo.hStdInput = handle_IN_Rd;
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

	stringstream cmdbuffer;
	int curargument = 0;
	while (arguments[curargument]) {
		cmdbuffer << arguments[curargument] << " ";
		curargument++;
	}


	stringstream envbuffer;
    map<string, string>::iterator iter;
    for (iter = env.begin(); iter != env.end(); ++iter) {
       // if (iter->first == "PWD") continue;
        envbuffer << iter->first << string("=") << iter->second << '\0';
    }

	envbuffer << '\0';

	LPCSTR curdir = directory.empty() ? NULL : directory.c_str();

	if (!CreateProcess(NULL, (char *) cmdbuffer.str().c_str(), NULL, NULL, true, 0,
		(void *)envbuffer.str().c_str(),
		curdir, &siStartInfo, &piProcInfo )) {

		std::cout << "Error: " << GetLastError()  << std::endl;
		cleanup();
		return false;
	}

	int wrfd = _open_osfhandle((intptr_t)handle_IN_Wr, 0);
	int rdfd = _open_osfhandle((intptr_t)handle_OUT_Rd, _O_RDONLY);

	if (wrfd == -1 || rdfd == -1) {
		stop();
		return false;
	}

    p_stdin = _fdopen(wrfd, "w");
    p_stdout = _fdopen(rdfd, "r");

	if (!p_stdin || !p_stdout) {
		stop();
		return false;
	}

#else

    if (pid) return false;

    pipe(out);
    pipe(in);

    posix_spawn_file_actions_init(&action);
    posix_spawn_file_actions_adddup2(&action, out[0], 0);
    posix_spawn_file_actions_addclose(&action, out[1]);

    posix_spawn_file_actions_adddup2(&action, in[1], 1);
    posix_spawn_file_actions_addclose(&action, in[0]);

    vector<string> vars;

    map<string, string>::iterator iter;
    for (iter = env.begin(); iter != env.end(); ++iter) {
       // if (iter->first == "PWD") continue;
        vars.push_back(iter->first + string("=") + iter->second);
    }

    std::vector<char *> vars_c(vars.size() + 1); 

    for (std::size_t i = 0; i != vars.size(); ++i) {
        vars_c[i] = &vars[i][0];
    }

    vars_c[vars.size()] = NULL;

    string cwd = __getcwd();

    if (directory.size() > 0)
        chdir(directory.c_str());

    if (posix_spawnp(&pid, program, &action, NULL, arguments, vars_c.data())) {
        cleanup();
        pid = 0;
        if (directory.size() > 0) chdir(cwd.c_str());
        return false;
    }

    if (directory.size() > 0) chdir(cwd.c_str());

    p_stdin = fdopen(out[1], "w");
    p_stdout = fdopen(in[0], "r");

#endif

    return true;

}

bool Process::stop() {

#ifdef WIN32

	if (!piProcInfo.hProcess) return NULL;

	if (TerminateProcess(piProcInfo.hProcess, 0))
        return true;
    else 
        return false;

	cleanup();

    piProcInfo.hProcess = 0;

#else

    if (!pid) return false;

    kill(pid, SIGTERM);

    cleanup();

    pid = 0;

#endif



    return true;
}
//GetExitCodeProcess
void Process::cleanup() {

#ifdef WIN32

	if (!piProcInfo.hProcess) return;

	CloseHandle(piProcInfo.hProcess);
	CloseHandle(piProcInfo.hThread);

	if (p_stdin)
		fclose(p_stdin);
	if (p_stdout)
		fclose(p_stdout);

	CloseHandle(handle_IN_Rd);
	CloseHandle(handle_IN_Wr);
	CloseHandle(handle_OUT_Rd);
	CloseHandle(handle_OUT_Wr);

#else
	if (!pid) return;

    close(out[0]);
    close(in[1]);
    close(out[1]);
    close(in[0]);

    posix_spawn_file_actions_destroy(&action);

#endif

}

FILE* Process::get_input() {

#ifdef WIN32
	if (!piProcInfo.hProcess) return NULL;
#else
    if (!pid) return NULL;
#endif

    return p_stdin;

}

FILE* Process::get_output() {

#ifdef WIN32
	if (!piProcInfo.hProcess) return NULL;
#else
    if (!pid) return NULL;
#endif

    return p_stdout;

}

bool Process::is_alive() {

#ifdef WIN32

	DWORD dwExitCode = 9999;

	if (GetExitCodeProcess(piProcInfo.hProcess, &dwExitCode))
		return dwExitCode == STILL_ACTIVE;
	else
		return false;

#else

    if (!pid) return false;

    if (0 == kill(pid, 0))
        return true;
    else 
        return false;

#endif

}

int Process::get_handle() {

#ifdef WIN32
	if (!piProcInfo.hProcess) return 0; else return (int) piProcInfo.hProcess;
#else
    if (!pid) return 0; else return pid;
#endif

}

void Process::set_directory(string pwd) {

    directory = pwd;

}

void Process::set_environment(string key, string value) {

    env[key] = value;

}

void Process::copy_environment() {
//GetEnvironmentVariable

#ifdef WIN32

	LPTSTR lpszVariable;
	LPCH lpvEnv;

	lpvEnv = GetEnvironmentStrings();

	if (lpvEnv == NULL) return;

	// Variable strings are separated by NULL byte, and the block is terminated by a NULL byte.
	for (lpszVariable = (LPTSTR) lpvEnv; *lpszVariable; lpszVariable++) {

        LPCH ptr = strchr(lpszVariable, '=');

        if (!ptr) continue;

        set_environment(string(lpszVariable, ptr-lpszVariable), string(ptr+1));

		lpszVariable += strlen(lpszVariable) ;

	}

	FreeEnvironmentStrings(lpvEnv);

#else

    for(char **current = environ; *current; current++) {
    
        char* var = *current;
        char* ptr = strchr(var, '=');
        if (!ptr) continue;

        set_environment(string(var, ptr-var), string(ptr+1));

    }

#endif
}

