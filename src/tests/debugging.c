

#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

void __cyg_profile_func_enter (void *, void *) __attribute__((no_instrument_function));
void __cyg_profile_func_exit (void *, void *) __attribute__((no_instrument_function));

void print_function(void* pfnFuncAddr)
{
    Dl_info info;
    memset(&info,0,sizeof(info));
    if(dladdr(pfnFuncAddr,&info) && info.dli_fname)
    {
    	printf("%s", info.dli_sname);
            /*here: 'info.dli_fname' contains the function name */
            /*      'info.dli_fbase' contains Address at which shared library is loaded */
    }
    else
		printf("%p", pfnFuncAddr);
}

static int depth = -1;

void __cyg_profile_func_enter (void *func,  void *caller)
{
 	int n;
	depth++;
	for (n = 0; n < depth; n++) printf ("  ");
	printf ("|-> ");
	print_function(func);
	printf ("\n");
}


void __cyg_profile_func_exit (void *func, void *caller)
{
/*	int n;

	printf ("<-");
	for (n = 0; n < depth; n++) printf ("-");
	print_function(func);
	printf ("\n");
*/
 	depth--;
}