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


int gport=0;
struct sockaddr_in gserv_addr;
/*void error(const char *msg)
{
    perror(msg);
    exit(0);

}*/


void sig_handler(int signo)
{
  if (signo == SIGINT)
    printf("received SIGINT\n");
	exit(0);
}



void SigCatcher(int n)
{
wait3(NULL,WNOHANG,NULL);
}
int main(int argc, char *argv[])
{
     int sockfd, portno;
    /* char buffer[256];*/
     struct sockaddr_in serv_addr, cli_addr;
     int n=0,pid;
    socklen_t clilen=sizeof(cli_addr);
    



	if (signal(SIGINT, sig_handler) == SIG_ERR)
  printf("\ncan't catch SIGINT\n");

	FILE *conf=NULL;
	char buf[1026];
	int a[2];
	int cnt=0;
	memset(buf, '\0', sizeof(buf));
	char *start_ptr,*tab_ptr,*tempstrn;
	conf = fopen(argv[1], "r");
		if (conf == NULL) 
		{
			fprintf(stderr, "Error in opening %s: %s for reading.\n",argv[2], strerror(errno));
			exit(0);
		}
	while(fgets(buf, sizeof(buf), conf) != NULL) 
	{
		if (buf[0] == '#')
		{
		continue;
		}
		start_ptr=buf;
		tab_ptr = strchr(start_ptr, ' ');
		start_ptr = tab_ptr++;
		tempstrn= strrchr(start_ptr,'\n');
		*tempstrn='\0';
		a[cnt]=atoi(start_ptr);
		printf(" %d %s \n",a[cnt],start_ptr);
		cnt++;
	}




     sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    /* if (sockfd < 0) 
        error("ERROR opening socket");*/
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
     serv_addr.sin_port = 0;
     bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
     socklen_t slen = sizeof(serv_addr);
     getsockname(sockfd, (struct sockaddr *)&serv_addr, &slen);
     portno = ntohs(serv_addr.sin_port);
     gserv_addr=serv_addr;
     printf("Proxy port number= %d\n",portno);/*prit this in the log file*/
     
	signal(SIGCHLD,SigCatcher);/*check where to insert this*/
     int nrot=a[1];
	
     for(n=0;n<nrot;n++)
	{
		pid=fork();
		/* if (pid < 0)
            		 error("ERROR on fork");*/
        	 if (pid == 0)
		  {		close(sockfd);
		  		routerroutine(a,nrot,gserv_addr);
        	  } 
		else/*the proxy(parent) should wait here to get a message from the child*/
		{	
			FILE *prof1;
                        char *mode = "w";
                        char fil[20];
                         sprintf(fil,"stage%d.proxy.out",a[0]);
                        prof1 = fopen(fil,mode);
			printf("proxy port from proxy %d \n", ntohs(serv_addr.sin_port));
			char buffer[256];
			char tun_name[IFNAMSIZ];
			fd_set readfds;
		  /* Connect to the device */
			  strcpy(tun_name,"tun1");
			  int tun_fd = tun_alloc(tun_name, IFF_TUN | IFF_NO_PI);  /* tun interface */
			  int maxfd = tun_fd > sockfd ? tun_fd:sockfd;
			  maxfd++;
			  if(tun_fd < 0){
			    perror("Allocating interface");
			    exit(1);
			  }
			  char buffert[2000];
                          bzero(buffert,2000);
                          bzero(buffer,256);
			  
			   int rv = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&cli_addr, &clilen);/*stage one :waiting for router "im up message"*/
				if (rv > 0)
				{
				printf("received message: %s\n", buffer);

				printf("router port from proxy %d \n", ntohs(cli_addr.sin_port));
				}
				fprintf(prof1,"proxy port: %d\nrouter: %d, pid: %d, port: %d\n",ntohs(serv_addr.sin_port),nrot,pid,ntohs(cli_addr.sin_port));
                          	fflush(prof1);
            		  while(1)
			  {	
				FD_ZERO(&readfds);
                          	FD_SET(sockfd, &readfds);
                          	FD_SET(tun_fd, &readfds);
				if (select(maxfd , &readfds, NULL, NULL,NULL ) == -1){
				perror("select error");
				}
				if( FD_ISSET(sockfd,&readfds))
				{
					 rv = recvfrom(sockfd, buffert, sizeof(buffert), 0, NULL, NULL);
                                         if (rv > 0)
         	                         {
						struct iphdr *ip = (struct iphdr*) buffert;
                                           struct icmphdr *icmp = (struct icmphdr*) (buffert + sizeof(struct iphdr));
                                                char str[INET_ADDRSTRLEN];
                                                struct in_addr soaddr = {ip->saddr};
                                                struct in_addr dsaddr = {ip->daddr};
                                                inet_ntop(AF_INET, &(dsaddr), str, INET_ADDRSTRLEN);
                                           printf("PROXY recvd from router:Read %d bytes from rt port %d : src %s dst %s and type is %d\n", rv, ntohs(cli_addr.sin_port),inet_ntoa(soaddr),str,icmp->type);
					 	if(a[0]>1)
						fprintf(prof1,"ICMP from port: %d src: %s, dst: %s, type: %d\n",ntohs(cli_addr.sin_port),inet_ntoa(soaddr),str,icmp->type);				
						fflush(prof1);
						write(tun_fd,buffert,rv);
			       		 }
				}
	   			if(FD_ISSET(tun_fd,&readfds))
				{
						int nread = read(tun_fd,buffert,sizeof(buffert));
						    if(nread < 0) {
					      perror("Reading from interface");
					      close(tun_fd);
					      exit(1);
					    }
					
					
					struct iphdr *ip = (struct iphdr*) buffert;
					   struct icmphdr *icmp = (struct icmphdr*) (buffert + sizeof(struct iphdr));
						char str[INET_ADDRSTRLEN];
						struct in_addr soaddr = {ip->saddr};
						struct in_addr dsaddr = {ip->daddr};
						inet_ntop(AF_INET, &(dsaddr), str, INET_ADDRSTRLEN);
					   printf("PROXYYYYYYY:Read %d bytes from device %s : src %s dst %s and type is %d\n", nread, tun_name,inet_ntoa(soaddr),str,icmp->type);
					if(a[0]>1)
					fprintf(prof1,"ICMP from tunnel: src: %s, dst: %s, type: %d\n",inet_ntoa(soaddr),str,icmp->type);
					fflush(prof1);				
						if (sendto(sockfd, buffert, nread, 0, (struct sockaddr *)&cli_addr, sizeof(cli_addr)) < 0) {
						perror("sendto failed");
						return 0;
						}
									
				}		
			
		}
		fclose(prof1);	
		}
}
return 0;
}

