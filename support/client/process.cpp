
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <fcntl.h>

#include "process.h"

#if defined(__OS2__) || defined(__WINDOWS__) || defined(WIN32) || defined(WIN64) || defined(_MSC_VER)

#include <io.h>
#include <process.h>

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

inline const string int_to_string(int i) {

    stringstream buffer;
    buffer << i;
    return buffer.str();

}


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
                s = i + 1;
            }
            else s = i;
            continue;
        }
        if (s >= 0 && ((!quotes && str[i] == ' ') || (quotes && str[i] == '"' && (i == start || str[i - 1] != '\''))))
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

    return str[e] == '\0' ? e : (quotes ? e + 2 : e + 1);
}

char** parse_command(const char* command) {

    int length = (int)strlen(command);
    char* buffer = new char[length+1];
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

    char** result = new char*[i + 1];
    for (int j = 0; j < i; j++) result[j] = tokens[j];
    result[i] = NULL;
    delete [] tokens;
    delete [] buffer;
    return result;

}


Process::Process(string command, bool explicit_mode) : running(false), explicit_mode(explicit_mode) {

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

    kill();

    cleanup();

    int i = 0;

    while (arguments[i]) {
        free(arguments[i]);
        i++;
    }

    delete [] arguments;

}

bool Process::start() {

    OBJECT_SYNCHRONIZE {

        if (running) return false;

        exit_status = -1;

        p_stdin = -1;
        p_stdout = -1;
        p_stderr = -1;

#if defined(WIN32)

        handle_IN_Rd = NULL;
        handle_IN_Wr = NULL;
        handle_OUT_Rd = NULL;
        handle_OUT_Wr = NULL;
        handle_ERR_Rd = NULL;
        handle_ERR_Wr = NULL;

        SECURITY_ATTRIBUTES saAttr;

        saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        saAttr.bInheritHandle = TRUE;
        saAttr.lpSecurityDescriptor = NULL;

        // Create a pipe for the child process's STDOUT.
        if ( ! CreatePipe(&handle_OUT_Rd, &handle_OUT_Wr, &saAttr, 0) )
            return false;

        // Create a pipe for the child process's STDERR.
        if ( ! CreatePipe(&handle_ERR_Rd, &handle_ERR_Wr, &saAttr, 0) )
            return false;

        // Create a pipe for the child process's STDIN.
        if (! CreatePipe(&handle_IN_Rd, &handle_IN_Wr, &saAttr, 0))
            return false;

        if (explicit_mode) {

            if ( ! SetHandleInformation(handle_IN_Rd, HANDLE_FLAG_INHERIT, 1) )
                return false;

            if ( ! SetHandleInformation(handle_OUT_Wr, HANDLE_FLAG_INHERIT, 1) )
                return false;

        }

        // Ensure the write handle to the pipe for STDIN is not inherited.
        if ( ! SetHandleInformation(handle_IN_Wr, HANDLE_FLAG_INHERIT, 0) )
            return false;

        // Ensure the read handle to the pipe for STDOUT is not inherited.
        if ( ! SetHandleInformation(handle_OUT_Rd, HANDLE_FLAG_INHERIT, 0) )
            return false;

        // Ensure the read handle to the pipe for STDERR is not inherited.
        if ( ! SetHandleInformation(handle_ERR_Rd, HANDLE_FLAG_INHERIT, 0) )
            return false;


        STARTUPINFO siStartInfo;

        // Set up members of the PROCESS_INFORMATION structure.

        ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );


        stringstream cmdbuffer;
        int curargument = 0;
        while (arguments[curargument]) {
            cmdbuffer << "\"" << arguments[curargument] << "\" ";
            curargument++;
        }


        stringstream envbuffer;
        map<string, string>::iterator iter;
        for (iter = env.begin(); iter != env.end(); ++iter) {
            envbuffer << iter->first << string("=") << iter->second << '\0';
        }

        // Set up members of the STARTUPINFO structure.
        // This structure specifies the STDIN and STDOUT handles for redirection.
        ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
        siStartInfo.cb = sizeof(STARTUPINFO);
        siStartInfo.dwFlags |= STARTF_USESHOWWINDOW;
        if (!explicit_mode) {
            siStartInfo.hStdError = handle_ERR_Wr;
            siStartInfo.hStdOutput = handle_OUT_Wr;
            siStartInfo.hStdInput = handle_IN_Rd;
            siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
        } else {
            siStartInfo.hStdError = handle_ERR_Wr;
            HANDLE pHandle = GetCurrentProcess();
            HANDLE handle_IN_Rd2, handle_OUT_Wr2;
            DuplicateHandle(pHandle, handle_IN_Rd, pHandle, &handle_IN_Rd2, DUPLICATE_SAME_ACCESS, true, DUPLICATE_SAME_ACCESS);
            DuplicateHandle(pHandle, handle_OUT_Wr, pHandle, &handle_OUT_Wr2, DUPLICATE_SAME_ACCESS, true, DUPLICATE_SAME_ACCESS);
            envbuffer << string("TRAX_IN=") << handle_IN_Rd2 << '\0';
            envbuffer << string("TRAX_OUT=") << handle_OUT_Wr2 << '\0';
        }

        envbuffer << '\0';

        LPCSTR curdir = directory.empty() ? NULL : directory.c_str();

        if (!CreateProcess(NULL, (char *) cmdbuffer.str().c_str(), NULL, NULL, true, CREATE_NO_WINDOW,
        (void *)envbuffer.str().c_str(),
        curdir, &siStartInfo, &piProcInfo )) {

            std::cerr << "Error: " << GetLastError()  << std::endl;
            running = true;
            cleanup();
            return false;
        }

        int wrfd = _open_osfhandle((intptr_t)handle_IN_Wr, 0);
        int rdfd = _open_osfhandle((intptr_t)handle_OUT_Rd, _O_RDONLY);
        int erfd = _open_osfhandle((intptr_t)handle_ERR_Rd, _O_RDONLY);

        if (wrfd == -1 || rdfd == -1) {
            stop();
            return false;
        }

        p_stdin = wrfd;
        p_stdout = rdfd;
        p_stderr = erfd;

        if (!p_stdin || !p_stdout) {
            stop();
            return false;
        }

#else

        action_initialized = false;

        if (pid) return false;

        if (pipe(out) == -1 || pipe(in) == -1 || pipe(err) == -1) {
            std::cerr << "Error: unable to configure process streams" << std::endl;
            return false;
        }

        vector<string> vars;

        map<string, string>::iterator iter;
        for (iter = env.begin(); iter != env.end(); ++iter) {
            // if (iter->first == "PWD") continue;
            vars.push_back(iter->first + string("=") + iter->second);
        }

        posix_spawn_file_actions_init(&action);
        action_initialized = true;
        posix_spawn_file_actions_addclose(&action, out[1]);
        posix_spawn_file_actions_addclose(&action, in[0]);
        posix_spawn_file_actions_addclose(&action, err[0]);

        if (!explicit_mode) {
            posix_spawn_file_actions_adddup2(&action, out[0], 0);
            posix_spawn_file_actions_adddup2(&action, in[1], 1);
            posix_spawn_file_actions_adddup2(&action, err[1], 2);
        } else {
            posix_spawn_file_actions_adddup2(&action, err[1], 2);
            vars.push_back(string("TRAX_OUT=") + int_to_string(in[1]));
            vars.push_back(string("TRAX_IN=") + int_to_string(out[0]));
        }

        std::vector<char *> vars_c(vars.size() + 1);

        for (std::size_t i = 0; i != vars.size(); ++i) {
            vars_c[i] = &vars[i][0];
        }

        vars_c[vars.size()] = NULL;

        string cwd = __getcwd();

#define CHDIR_SOFT(D) \
        { if (chdir((D).c_str()) == -1) { \
            std::cerr << "Error: unable to switch to working directory" << std::endl; \
        } }

        if (directory.size() > 0)
            CHDIR_SOFT(directory);

        if (posix_spawnp(&pid, program, &action, NULL, arguments, vars_c.data())) {
            running = true;
            cleanup();
            pid = 0;
            if (directory.size() > 0) CHDIR_SOFT(cwd);
            return false;
        }

        if (directory.size() > 0) CHDIR_SOFT(cwd);

        p_stdin = out[1];
        p_stdout = in[0];
        p_stderr = err[0];

#endif

        running = true;

        return true;

    }

    return false;
}

bool Process::stop() {

    OBJECT_SYNCHRONIZE {

        bool result = true;

        if (running) {

#ifdef WIN32

            DWORD dwProcessID = piProcInfo.dwProcessId;

            for (HWND hwnd = GetTopWindow(NULL); hwnd; hwnd = ::GetNextWindow(hwnd, GW_HWNDNEXT))
            {
                DWORD dwWindowProcessID;
                DWORD dwThreadID = ::GetWindowThreadProcessId(hwnd, &dwWindowProcessID);
                if (dwWindowProcessID == dwProcessID)
                    PostThreadMessage(dwThreadID, WM_QUIT, 0, 0);
            }

            is_alive();

#else

            ::kill(pid, SIGTERM);

            is_alive();

#endif

        }

        return result;

    }

    return false;
}

bool Process::kill() {

    OBJECT_SYNCHRONIZE {

        bool result = true;

        if (running) {

#ifdef WIN32

            result = TerminateProcess(piProcInfo.hProcess, -15);
            is_alive();

#else

            ::kill(pid, SIGKILL);
            is_alive();

#endif

        }

        return result;

    }

    return false;
}

//GetExitCodeProcess
void Process::cleanup() {

    OBJECT_SYNCHRONIZE {

#ifdef WIN32

#define CLOSE_AND_RESET(H) { if (H) { CloseHandle((H)); H = NULL; }  }

        CLOSE_AND_RESET(piProcInfo.hProcess);
        CLOSE_AND_RESET(piProcInfo.hThread);

        if (p_stdin != -1) {
            close(p_stdin);
            p_stdin = -1;
        }
        if (p_stdout != -1) {
            close(p_stdout);
            p_stdout = -1;
        }
        if (p_stderr != -1) {
            close(p_stderr);
            p_stderr = -1;
        }

        CLOSE_AND_RESET(handle_IN_Rd);
        CLOSE_AND_RESET(handle_IN_Wr);
        CLOSE_AND_RESET(handle_OUT_Rd);
        CLOSE_AND_RESET(handle_OUT_Wr);
        CLOSE_AND_RESET(handle_ERR_Rd);
        CLOSE_AND_RESET(handle_ERR_Wr);

#else

        if (out[0] != -1) {close(out[0]); out[0] = -1; };
        if (err[0] != -1) {close(err[0]); err[0] = -1; };
        if (in[1] != -1) {close(in[1]); in[1] = -1; };
        if (out[1] != -1) {close(out[1]); out[1] = -1; };
        if (err[1] != -1) {close(err[1]); err[1] = -1; };
        if (in[0] != -1) {close(in[0]); in[0] = -1; };

        if (action_initialized) {
            posix_spawn_file_actions_destroy(&action);
            action_initialized = false;
        }

#endif

        running = false;

    }
}

int Process::get_input() {

    OBJECT_SYNCHRONIZE {

        if (running) return p_stdin;

    }

    return -1;

}

int Process::get_output() {

    OBJECT_SYNCHRONIZE {

        if (running) return p_stdout;

    }

    return -1;
}

int Process::get_error() {

    OBJECT_SYNCHRONIZE {

    // On Visual Studio there seem to be problems with stderr forwarding,
    // so it is disabled for now.
#ifdef _MSC_VER
        return -1;
#endif

        if (running) return p_stderr;

    }

    return -1;
}

bool Process::is_alive(int *status) {

    OBJECT_SYNCHRONIZE {

        if (status) *status = 0;

#ifdef WIN32

        DWORD dwExitCode = STILL_ACTIVE;

        if (GetExitCodeProcess(piProcInfo.hProcess, &dwExitCode)) {
            if (status) *status = dwExitCode;
            running = dwExitCode == STILL_ACTIVE;
            return running;
        } else
            return false;

#else

        if (!running) {
            if (status) *status = exit_status;
            return false;
        }

        int childExitStatus;
        int result = waitpid(pid, &childExitStatus, WNOHANG);
        if (result <= 0) {
            return result == 0;
        }

        if (WIFEXITED(childExitStatus)) {
            exit_status = WEXITSTATUS(childExitStatus);
            running = false;
        } else if (WIFSIGNALED(childExitStatus)) {
            exit_status = -WTERMSIG(childExitStatus);
            running = false;
        } else {
            return true;
        }

        if (status) *status = exit_status;

        return false;
#endif

    }

    return false;
}

int Process::get_handle() {

    OBJECT_SYNCHRONIZE {

        if (!running) return 0;

#ifdef WIN32
        return (int) GetProcessId(piProcInfo.hProcess);
#else
        return pid;
#endif

    }

    return 0;

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

        set_environment(string(lpszVariable, ptr - lpszVariable), string(ptr + 1));

        lpszVariable += strlen(lpszVariable) ;

    }

    FreeEnvironmentStrings(lpvEnv);

#else

    for (char **current = environ; *current; current++) {

        char* var = *current;
        char* ptr = strchr(var, '=');
        if (!ptr) continue;

        set_environment(string(var, ptr - var), string(ptr + 1));

    }

#endif
}

