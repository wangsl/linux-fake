
// https://manpages.ubuntu.com/manpages/xenial/man1/uid_wrapper.1.html
// gcc -fPIC -shared -Wl,-soname,libfakeuid.so -ldl -o libfakeuid.so fakeuid.c

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include <unistd.h>

#ifndef RTLD_NEXT
#define RTLD_NEXT ((void *) -1l)
#endif

#define ENV_VARNAME_FAKE_UID "FAKE_UID"
#define ENV_VARNAME_FAKE_EUID "FAKE_EUID"
#define ENV_VARNAME_FAKE_GID "FAKE_GID"
#define ENV_VARNAME_FAKE_EGID "FAKE_EGID"

#define FAKE_UID 12345

typedef	uid_t (*__libc_getuid)(void);
typedef uid_t (*__libc_geteuid)(void);
typedef gid_t (*__libc_getgid)(void);
typedef gid_t (*__libc_getegid)(void);

static __libc_getuid __orig_getuid = NULL;
static __libc_geteuid __orig_geteuid = NULL;
static __libc_getgid __orig_getgid = NULL;
static __libc_getegid __orig_getegid = NULL;

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

uid_t getuid()
{
  if(!__orig_getuid) __orig_getuid = get_lib_function("getuid");
  uid_t my_uid = __orig_getuid();
  if(getenv(ENV_VARNAME_FAKE_UID)) my_uid = atoi(getenv(ENV_VARNAME_FAKE_UID));
  return my_uid;
}

uid_t geteuid()
{
  if(!__orig_geteuid) __orig_geteuid = get_lib_function("geteuid");
  uid_t my_euid = __orig_geteuid();
  if(getenv(ENV_VARNAME_FAKE_EUID)) my_euid = atoi(getenv(ENV_VARNAME_FAKE_EUID));
  return my_euid;
}

gid_t getgid()
{
  if(!__orig_getgid) __orig_getgid = get_lib_function("getgid");
  gid_t my_gid = __orig_getgid();
  if(getenv(ENV_VARNAME_FAKE_GID)) my_gid = atoi(getenv(ENV_VARNAME_FAKE_GID));
  return my_gid;
}

gid_t getegid()
{
  if(!__orig_getegid) __orig_getegid = get_lib_function("getegid");
  gid_t my_egid = __orig_getegid();
  if(getenv(ENV_VARNAME_FAKE_EGID)) my_egid = atoi(getenv(ENV_VARNAME_FAKE_EGID));
  return my_egid;
}
