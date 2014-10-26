#include <stdlib.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/ip.h>
#include <linux/icmp.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <errno.h>

#include "tunnel.h"

void error(const char *msg)
{
    perror(msg);
    exit(0);

}

void routerroutine(int a[], int nrot, struct sockaddr_in gserv_addr)
{
			int newsockfd1;
			struct sockaddr_in cli_addr1;
			FILE *rouf2;
        	char *mode = "w";
			char fil2[20];
			sprintf(fil2,"stage%d.router%d.out",a[0],nrot);
			rouf2 = fopen(fil2,mode);
			int gpid=getpid();
            int rv;
			char recbuf[2000];
			bzero(recbuf,2000);
			newsockfd1 = socket(AF_INET, SOCK_DGRAM, 0);
     		if (newsockfd1 < 0)
        		error("ERROR opening socket");		
		    cli_addr1.sin_family = AF_INET;
 	        cli_addr1.sin_addr.s_addr = htonl(INADDR_ANY);
  		    cli_addr1.sin_port = 0;
 			bind(newsockfd1, (struct sockaddr *)&cli_addr1, sizeof(cli_addr1));
 			socklen_t clen = sizeof(cli_addr1);
 			getsockname(newsockfd1, (struct sockaddr *)&cli_addr1, &clen);
			printf("router port issssss%d \n", ntohs(cli_addr1.sin_port));
			printf("proxy port from router %d \n", ntohs(gserv_addr.sin_port));
			/*fprintf(rouf1,"router: %d, pid: %d, port: %d",nrot,gpid,ntohs(cli_addr.sin_port));*/
			fprintf(rouf2,"router: %d, pid: %d, port: %d\n",nrot,gpid,ntohs(cli_addr1.sin_port));
			fflush(rouf2);
			char *buffer="im up"; 
    		sleep(0.5);
     		if(sendto(newsockfd1, buffer, strlen(buffer), 0, (struct sockaddr *)&gserv_addr, sizeof(gserv_addr)) < 0)
     		{
	    		perror("sendto failed");
	     		exit(0);
     		}
		printf("router port issssss%d \n", ntohs(cli_addr1.sin_port));
			while(1)
			{
			        rv = recvfrom(newsockfd1, recbuf, sizeof(recbuf), 0, NULL, NULL);
	                if (rv > 0)
	                {
	                	struct iphdr *ip = (struct iphdr*) recbuf;
	                    struct icmphdr *icmp = (struct icmphdr*) (recbuf+ sizeof(struct iphdr));	
	                    char str[INET_ADDRSTRLEN];
						char tempo1[INET_ADDRSTRLEN];
			    		char tempo2[INET_ADDRSTRLEN];
                        struct in_addr soaddr = {ip->saddr};
						struct in_addr dsaddr = {ip->daddr};
						inet_ntop(AF_INET, &(soaddr), tempo1, INET_ADDRSTRLEN);
						inet_ntop(AF_INET, &(dsaddr), tempo2, INET_ADDRSTRLEN);
						if(a[0]>1)
						fprintf(rouf2,"ICMP from port: %d, src: %s, dst: %s, type: %d\n",ntohs(gserv_addr.sin_port),tempo1,tempo2,icmp->type);
						fflush(rouf2);		   
				    	ip->saddr=ip->daddr;
				    	ip->daddr=soaddr.s_addr;
				    	soaddr.s_addr = ip->saddr;
				    	dsaddr.s_addr = ip->daddr;
				    	icmp->type=0;
				    	icmp->checksum=0;
				/*checksum calculation*/
				    	int len=rv-20;
				    	u_short *icm=((u_short *)recbuf+ sizeof(struct iphdr));
				    	int check=in_cksum(icm,len);
				    	icmp->checksum = check;	
				    	inet_ntop(AF_INET, &(dsaddr), str, INET_ADDRSTRLEN);
				    	printf("Router:Read %d bytes src %s dst %s and type is %d\n", rv,inet_ntoa(soaddr),str,icmp->type);
		         		if (sendto(newsockfd1, recbuf, rv, 0, (struct sockaddr *)&gserv_addr, sizeof(gserv_addr)) < 0)
		         		{
                        	perror("sendto failed");
                        	exit(0);
                        }
               
				}
			}
			fclose(rouf2);
			close(newsockfd1);
            	 	exit(0);
       	  }
