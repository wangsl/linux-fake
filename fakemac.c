// https://gist.github.com/mad4j/8864135
// https://stackoverflow.com/questions/1779715/how-to-get-mac-address-of-your-machine-using-a-c-program
// https://gist.github.com/MarisElsins/5e476327640dceb0f1a612267e09d89b
// gcc -fPIC -shared -Wl,-soname,libfakemac.so -ldl -o libfakemac.so fakemac.c

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <stdbool.h>

#include <sys/socket.h>
//#include <linux/if.h>
#include <netdb.h>
#include <assert.h>
#include <linux/sockios.h>

#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <linux/if_link.h>
#include <net/if.h>
#include <errno.h>

#ifndef RTLD_NEXT
#define RTLD_NEXT ((void *) -1l)
#endif

#define ENV_VARNAME_FAKE_MAC "FAKE_MAC"

typedef int (*ioctl_t)(int __fd, unsigned long __request, struct ifreq *s);
static ioctl_t __original_ioctl = NULL;

typedef int (*getifaddrs_t)(struct ifaddrs **);
static getifaddrs_t __original_getifaddrs = NULL;

typedef void (*freeifaddrs_t)(struct ifaddrs *);
static freeifaddrs_t __original_freeifaddrs = NULL;

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
  if(!__original_ioctl) __original_ioctl = (ioctl_t) get_lib_function("ioctl");

  int ret_val = __original_ioctl(fd, request, s);

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

int getifaddrs(struct ifaddrs **ifap) {

  if(!__original_getifaddrs) __original_getifaddrs = (getifaddrs_t) get_lib_function("getifaddrs");

  int result = __original_getifaddrs(ifap);
  if (result != 0) {
    perror("original getifaddrs");
    return result;
  }

  struct ifaddrs *ifa = *ifap;
  while(ifa) {
    //printf("Interface: %s\n", ifa->ifa_name);

    //Check for AF_INET (IPv4)
    // if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {
    //     struct sockaddr_in *addr = (struct sockaddr_in *)ifa->ifa_addr;
    //     printf("  IPv4 Address: %s\n", inet_ntoa(addr->sin_addr));
    // }

    // Check for AF_PACKET (MAC address)
    if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_PACKET) {
      struct sockaddr_ll *s = (struct sockaddr_ll *) ifa->ifa_addr;
      // printf("  MAC Address: %02x:%02x:%02x:%02x:%02x:%02x\n",
      //        s->sll_addr[0], s->sll_addr[1], s->sll_addr[2],
      //        s->sll_addr[3], s->sll_addr[4], s->sll_addr[5]);
      initialize_fake_mac_addesses();
      for(int i = 0; i < n_fake_mac_addresses; i++) {
        if(!strcmp(fake_mac_addresses[i].name, ifa->ifa_name)) {
          for(int j = 0; j < 6; j++) {
            s->sll_addr[j] = fake_mac_addresses[i].address[j];
          }
        }
      }
    }
    ifa = ifa->ifa_next;
  }
  return result;
}

// Custom freeifaddrs (needed for compatibility)
void freeifaddrs(struct ifaddrs *ifa) {
  if (!__original_freeifaddrs) __original_freeifaddrs = (freeifaddrs_t) get_lib_function("freeifaddrs");
  __original_freeifaddrs(ifa);
}
