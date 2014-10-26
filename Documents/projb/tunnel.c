#include <stdlib.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/ip.h>
#include <linux/icmp.h>
#include <string.h> 
#include "tunnel.h"
#include <stdio.h>
/* memset */
/*#include "../utility/socket.h"
#include "../utility/fileUtil.h"
#include "../utility/utility.h"
#include "../utility/networkUtil.h"
#include "../data/routerData.h"
#include "../control/controlMessage.h"
#include "../data/flowCache.h"*/


/*This function taken from http://backreference.org/2010/03/26/tuntap-interface-tutorial as suggested by the project description*/
int tun_alloc(char *dev, int flags) {

  struct ifreq ifr;
  int fd, err;
  char *clonedev = (char*)"/dev/net/tun";
  printf("dev is %s\n", dev);
  /* Arguments taken by the function:
 *    *
 *       * char *dev: the name of an interface (or '\0'). MUST have enough
 *          *   space to hold the interface name if '\0' is passed
 *             * int flags: interface flags (eg, IFF_TUN etc.)
 *                */

   /* open the clone device */
   if( (fd = open(clonedev, O_RDWR)) < 0 ) {
     return fd;
   }

   /* preparation of the struct ifr, of type "struct ifreq" */
   memset(&ifr, 0, sizeof(ifr));

   ifr.ifr_flags = flags;   /* IFF_TUN or IFF_TAP, plus maybe IFF_NO_PI */

   if (*dev) {
     /* if a device name was specified, put it in the structure; otherwise,
 *       * the kernel will try to allocate the "next" device of the
 *             * specified type */
     strncpy(ifr.ifr_name, dev, IFNAMSIZ);
   }

   /* try to create the device */
   if( (err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0 ) {
     close(fd);
     return err;
   }

  /* if the operation was successful, write back the name of the
 *    * interface to the variable "dev", so the caller can know
 *       * it. Note that the caller MUST reserve space in *dev (see calling
 *          * code below) */
  strcpy(dev, ifr.ifr_name);
	printf("dev is %s\n", dev);
  /* this is the special file descriptor that the caller will use to talk
 *    * with the virtual interface */
  return fd;
}
