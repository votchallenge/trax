
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stdexcept>

#include "process.h"

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

    int length = strlen(command);
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


Process::Process(string command) : pid(0) {

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

    return true;
}

bool Process::stop() {

    if (!pid) return false;

    kill(pid, SIGTERM);

    cleanup();

    pid = 0;

    return true;
}
//GetExitCodeProcess
void Process::cleanup() {

    if (!pid) return;

    close(out[0]);
    close(in[1]);
    close(out[1]);
    close(in[0]);

    posix_spawn_file_actions_destroy(&action);

}

FILE* Process::get_input() {

    if (!pid) return NULL;

    return p_stdin;

}

FILE* Process::get_output() {

    if (!pid) return NULL;

    return p_stdout;

}

bool Process::is_alive() {

    if (!pid) return false;

    int status;

    if (0 == kill(pid, 0))
        return true;
    else 
        return false;

}

int Process::get_handle() {

    if (!pid) return 0;

    return pid;

}

void Process::set_directory(string pwd) {

    directory = pwd;

}

void Process::set_environment(string key, string value) {

    env[key] = value;

}

void Process::copy_environment() {
//GetEnvironmentVariable


    for(char **current = environ; *current; current++) {
    
        char* var = *current;
        char* ptr = strchr(var, '=');
        if (!ptr) continue;

        set_environment(string(var, ptr-var), string(ptr+1));

    }

}

