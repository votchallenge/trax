/*******************************************************************************
* Copyright (c) 2013, Luka Cehovin
* All rights reserved.
*
* This is a private development version of Legit library. Free distribution of
* the code is not allowed without explicit permission from the copyright holders.
*******************************************************************************/
/*
POSIX getopt for Windows

AT&T Public License

Code given out at the 1985 UNIFORUM conference in Dallas.
*/

#ifdef __GNUC__
#include <getopt.h>
#endif
#ifndef __GNUC__

#ifndef _WINGETOPT_H_
#define _WINGETOPT_H_

#ifdef __cplusplus
extern "C" {
#endif

  extern int opterr;
  extern int optind;
  extern int optopt;
  extern char *optarg;
  extern int getopt(int argc, char **argv, char *opts);

#ifdef __cplusplus
}
#endif

#endif  /* _GETOPT_H_ */
#endif  /* __GNUC__ */