// https://gist.github.com/mad4j/8864135
// https://stackoverflow.com/questions/1779715/how-to-get-mac-address-of-your-machine-using-a-c-program
// https://gist.github.com/MarisElsins/5e476327640dceb0f1a612267e09d89b
// gcc -fPIC -shared -Wl,-soname,libfakemac.so -ldl -o libfakemac.so fakemac.c

/*
[wang@cs001 fakehostname]$ ifconfig eno1
eno1: flags=4099<UP,BROADCAST,MULTICAST>  mtu 1500
        ether 8c:0f:6f:60:06:77  txqueuelen 1000  (Ethernet)
        RX packets 0  bytes 0 (0.0 B)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 0  bytes 0 (0.0 B)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

[wang@cs001 fakehostname]$ LD_PRELOAD=libfakemac.so FAKE_MAC="eno1=8c:0f:6f:60:06:88" ifconfig eno1
eno1: flags=4099<UP,BROADCAST,MULTICAST>  mtu 1500
        ether 8c:0f:6f:60:06:88  txqueuelen 1000  (Ethernet)
        RX packets 0  bytes 0 (0.0 B)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 0  bytes 0 (0.0 B)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

[wang@cs001 fakehostname]$ 
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <stdbool.h>

#include <sys/socket.h>
#include <linux/if.h>
#include <netdb.h>
#include <assert.h>
#include <linux/sockios.h>

#ifndef RTLD_NEXT
#define RTLD_NEXT ((void *) -1l)
#endif

#define ENV_VARNAME_FAKE_MAC "FAKE_MAC"

typedef int (*ioctl_t)(int __fd, unsigned long __request, struct ifreq *s);

static ioctl_t __orig_ioctl = NULL;

#define MAX_FAKE_MAC_ADDRESSES 64
static int n_fake_mac_addresses = 0;
static struct {
  char name[32];
  unsigned char address[6];
} fake_mac_addresses[MAX_FAKE_MAC_ADDRESSES];

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

static void initialize_fake_mac_addesses()
{
  if(n_fake_mac_addresses > 0) return;

  const char *envs = getenv(ENV_VARNAME_FAKE_MAC); 
  if(envs == NULL) return;

  char *token = strtok((char *)envs, ";");
  int i = 0;
  while(token) {
    int n = sscanf(token, "%[A-Za-z0-9_-]=%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
      fake_mac_addresses[i].name,
      &fake_mac_addresses[i].address[0],
      &fake_mac_addresses[i].address[1],
      &fake_mac_addresses[i].address[2],
      &fake_mac_addresses[i].address[3],
      &fake_mac_addresses[i].address[4],
      &fake_mac_addresses[i].address[5]
    );
    if(n == 7) { 
      i++; 
      n_fake_mac_addresses = i; 
      assert(n_fake_mac_addresses < MAX_FAKE_MAC_ADDRESSES);
    }
    token = strtok(NULL, ";");
  }
}

/*
https://man7.org/linux/man-pages/man7/netdevice.7.html
SIOCGIFHWADDR, SIOCSIFHWADDR
  Get or set the hardware address of a device using
  ifr_hwaddr.  The hardware address is specified in a struct
  sockaddr.  sa_family contains the ARPHRD_* device type,
  sa_data the L2 hardware address starting from byte 0.
  Setting the hardware address is a privileged operation.
*/

int ioctl(int fd, unsigned long request, struct ifreq *s)
{
  if(!__orig_ioctl) __orig_ioctl = (ioctl_t) get_lib_function("ioctl");

  int ret_val = __orig_ioctl(fd, request, s);

  if(request == SIOCGIFHWADDR) {
    initialize_fake_mac_addesses();
    for(int i = 0; i < n_fake_mac_addresses; i++) {
      if(!strcmp(fake_mac_addresses[i].name, s->ifr_name)) {
        for(int j = 0; j < 6; j++) {
          s->ifr_addr.sa_data[j] = fake_mac_addresses[i].address[j];
        }
      }
    }
  }

  return ret_val;  
}
