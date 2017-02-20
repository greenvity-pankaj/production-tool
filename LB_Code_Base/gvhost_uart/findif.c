#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <inttypes.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/types.h>
#include <malloc.h>
#include <memory.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h> 
#include <sys/types.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>


#include "findif.h"


unsigned char find_interface(char * identified_ip_addr, char * identified_name, const char * search_pattern) {
	int fd;
	struct if_nameindex *curif, *ifs;
	struct ifreq req;

	memset(identified_name,0,IFNAMSIZ);
	memset(identified_ip_addr,0,IFNAMSIZ);
	if ((fd = socket(PF_INET, SOCK_DGRAM, 0)) != -1) {
		ifs = (struct if_nameindex *)if_nameindex();
		if (ifs) {
			for (curif = ifs; curif && curif->if_name ; curif++) {
				strncpy(req.ifr_name, curif->if_name, IFNAMSIZ);
				req.ifr_name[IFNAMSIZ] = 0;
				if (ioctl(fd, SIOCGIFADDR, &req) < 0)
					perror("ioctl"); //Getting Error here
				else
					if(strstr(curif->if_name, search_pattern) == NULL) {
						continue;
					} else {
						strcpy(identified_name,curif->if_name);
						strcpy(identified_ip_addr,inet_ntoa(((struct sockaddr_in*) &req.ifr_addr)->sin_addr));
						close(fd);
						return 1;
					}
			}
			if_freenameindex(ifs);
			if (close(fd) != 0) {
				perror("close");
				return 0;
			}
		} else {
			close(fd);
			perror("if_nameindex");
			return 0;
		}
	} else {
		close(fd);
		perror("socket");
		return 0;
	}
	
	close(fd);
	return 0; //return value (0 = not found, 1 = found)
}

