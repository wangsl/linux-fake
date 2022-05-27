
// Downloaded from: https://gist.github.com/MarisElsins/5e476327640dceb0f1a612267e09d89b
// gcc -fPIC -shared -Wl,-soname,libfakehostname.so -ldl -o libfakehostname.so fakehostname.c

/*
[wang@cs001 ~]$ LD_PRELOAD=/home/wang/fake/fakehostname/libfakehostname.so FAKE_HOSTNAME=myfakehostname.domain.com hostname 
myfakehostname.domain.com
[wang@cs001 ~]$ LD_PRELOAD=/home/wang/fake/fakehostname/libfakehostname.so FAKE_HOSTNAME=myfakehostname.domain.com uname -a
Linux myfakehostname.domain.com 4.18.0-305.28.1.el8_4.x86_64 #1 SMP Mon Nov 8 07:45:47 EST 2021 x86_64 x86_64 x86_64 GNU/Linux
[wang@cs001 ~]$ 
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <stdbool.h>

#ifndef RTLD_NEXT
#define RTLD_NEXT ((void *) -1l)
#endif

#define ENV_VARNAME_FAKE_HOSTNAME "FAKE_HOSTNAME"

typedef int (*uname_t) (struct utsname * buf);
typedef int (*gethostname_t)(char *name, size_t len);

static bool error_is_printed = false;
static uname_t __orig_uname = NULL;
static gethostname_t __orig_gethostname = NULL;

static void *get_lib_function(const char *funcname)
{
  void *func = dlsym(RTLD_NEXT, funcname);
  if(func == NULL) {
    char *error = dlerror();
    fprintf(stderr, "Failed to locate lib function '%s' error: %s", funcname, error);
      exit(EXIT_FAILURE);
  }
  return func;
}

static void *check_fake_host()
{
  if(!error_is_printed && getenv(ENV_VARNAME_FAKE_HOSTNAME) && 
    strlen(getenv(ENV_VARNAME_FAKE_HOSTNAME)) > sysconf(_SC_HOST_NAME_MAX)) {
    fprintf(stderr, "WARNING! '%s' variable exceeds %d (HOST_NAME_MAX) characters.\n", 
      ENV_VARNAME_FAKE_HOSTNAME, sysconf(_SC_HOST_NAME_MAX));
    error_is_printed = true;
  }
}

int gethostname(char *name, size_t len) 
{
  if(__orig_gethostname == NULL) {
    __orig_gethostname = (gethostname_t) get_lib_function("gethostname");
    check_fake_host();
  }

  int ret = __orig_gethostname(name, len);

  if(getenv(ENV_VARNAME_FAKE_HOSTNAME)) 
    strncpy(name, getenv(ENV_VARNAME_FAKE_HOSTNAME), len);

  return ret;
}

int uname(struct utsname *buf)
{
  if(__orig_uname == NULL) {
    __orig_uname = (uname_t) get_lib_function("uname");
    check_fake_host();
  }

  int ret = __orig_uname((struct utsname *) buf);

  if(getenv(ENV_VARNAME_FAKE_HOSTNAME)) {
    memset(buf->nodename, 0, sizeof(buf->nodename));
    strncpy(buf->nodename, getenv(ENV_VARNAME_FAKE_HOSTNAME), sizeof(buf->nodename)-1);
  }

  return ret;
}
