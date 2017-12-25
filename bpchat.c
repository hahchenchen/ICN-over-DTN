/*
 * bpchat.c
 * Andrew Jenkins <andrew.jenkins@colorado.edu>
 * Reads lines from stdin and sends those in bundles.
 * Receives bundles and writes them to stdout.
 */

#include <stdlib.h>
#include <stdio.h>
#include <bp.h>

#include <sys/un.h>  
#include <unistd.h>

static BpSAP                sap;
static Sdr                  sdr;
static pthread_mutex_t      sdrmutex = PTHREAD_MUTEX_INITIALIZER;
static char                 *destEid = NULL;
static char                 *ownEid = NULL;
static BpCustodySwitch      custodySwitch = NoCustodyRequested;
static int                  running = 1;

const char usage[] =
"Usage: bpchat.c <source EID> <dest EID> [ct]\n\n"
"Reads lines from stdin and sends these lines in bundles.\n"
"Receives bundles and writes them to stdout.\n"
"If \"ct\" is specified, sent bundles have the custody transfer flag set\n";

static pthread_t    sendLinesThread;
static void *       sendLines(void *args)
{
	Object          bundleZco, bundlePayload;
	Object          newBundle;   /* We never use but bp_send requires it. */
	int             lineLength = 0;
	char            lineBuffer[1024];

	int sockfd,newfd,ret;  
 
    struct sockaddr_un server_addr;  
   

	while(running) {
		 remove("/home/istin/server.socket");/*不管有没有，先删除一下，否则如果该文件已经存在的的话，bind会失败。*/  
		memset(&server_addr,0,sizeof(server_addr));  
		server_addr.sun_family=AF_UNIX;  
		strcpy(server_addr.sun_path,"/home/istin/server.socket");  
		sockfd=socket(AF_UNIX,SOCK_STREAM,0);  
		if (sockfd<0)  
		{  
		printf("调用socket函数建立socket描述符出错！\n");  
		 exit(1);  
		}  
		printf("调用socket函数建立socket描述符成功！\n");  
		ret=bind(sockfd,(struct sockaddr *)(&server_addr),sizeof(server_addr));  
		if (ret<0)  
		{  
		printf("调用bind函数绑定套接字与地址出错！\n");  
		 exit(2);  
		}  
		printf("调用bind函数绑定套接字与地址成功！\n");  
		chmod(server_addr.sun_path,S_IRWXU | S_IRWXG | S_IRWXO);
		ret=listen(sockfd,4);  
		if (ret<0)  
		{  
		printf("调用listen函数出错，无法宣告服务器已经可以接受连接！\n");  
		 exit(3);  
		}  
		printf("调用listen函数成功，宣告服务器已经可以接受连接请求！\n");  
		newfd=accept(sockfd,NULL,NULL);/*newfd连接到调用connect的客户端*/  
		if (newfd<0)  
		{  
		printf("调用accept函数出错，无法接受连接请求，建立连接失败！\n");  
		 exit(4);  
		}  
		printf("调用accept函数成功，服务器与客户端建立连接成功！\n");  
		bzero(lineBuffer,sizeof(lineBuffer));
		if(recv(newfd,lineBuffer,sizeof(lineBuffer),0)<0)
		{
			printf("调用recv接受失败！\n"); 
			fprintf(stderr, "EOF\n");
			running = 0;
			bp_interrupt(sap);
			break; 
		} 
            
		/* Read a line from stdin */
	/*	if(fgets(lineBuffer, sizeof(lineBuffer), stdin) == NULL) {
			fprintf(stderr, "EOF\n");
			running = 0;
			bp_interrupt(sap);
			break;
		}*/
		printf("data is:%s\n", lineBuffer);
		lineLength = strlen(lineBuffer);

		/* Wrap the linebuffer in a bundle payload. */
		if(pthread_mutex_lock(&sdrmutex) != 0)
		{
			putErrmsg("Couldn't take sdr mutex.", NULL);
			break;
		}

		oK(sdr_begin_xn(sdr));
		bundlePayload = sdr_malloc(sdr, lineLength);
		if(bundlePayload) {
			sdr_write(sdr, bundlePayload, lineBuffer, lineLength);
		}

		if(sdr_end_xn(sdr) < 0) {
			pthread_mutex_unlock(&sdrmutex);
			bp_close(sap);
			putErrmsg("No space for bpchat payload.", NULL);
			break;
		}

		bundleZco = ionCreateZco(ZcoSdrSource, bundlePayload, 0, 
			lineLength, BP_STD_PRIORITY, 0, ZcoOutbound, NULL);
		if(bundleZco == 0 || bundleZco == (Object) ERROR) {
			pthread_mutex_unlock(&sdrmutex);
			bp_close(sap);
			putErrmsg("bpchat can't create bundle ZCO.", NULL);
			break;
		}
		pthread_mutex_unlock(&sdrmutex);

		/* Send the bundle payload. */
		if(bp_send(sap, destEid, NULL, 86400, BP_STD_PRIORITY,
				custodySwitch, 0, 0, NULL, bundleZco,
				&newBundle) <= 0)
		{
			putErrmsg("bpchat can't send bundle.", NULL);
			break;
		}
	}
	return NULL;
}

static pthread_t    recvBundlesThread;
static void *       recvBundles(void *args)
{
	BpDelivery      dlv;
	ZcoReader       reader;
	char            buffer[1024];
	int             bundleLenRemaining;
	int             rc;
	int             bytesToRead;

	while(running)
	{
		if(bp_receive(sap, &dlv, BP_BLOCKING) < 0)
		{
			putErrmsg("bpchat bundle reception failed.", NULL);
			break;
		}

		if(dlv.result == BpReceptionInterrupted || dlv.adu == 0)
		{
			bp_release_delivery(&dlv, 1);
			continue;
		}

		if(dlv.result == BpEndpointStopped)
		{
			bp_release_delivery(&dlv, 1);
			break;
		}

		if(pthread_mutex_lock(&sdrmutex) != 0)
		{
			putErrmsg("Couldn't take sdr mutex.", NULL);
			break;
		}

		oK(sdr_begin_xn(sdr));
		bundleLenRemaining = zco_source_data_length(sdr, dlv.adu);
		zco_start_receiving(dlv.adu, &reader);
		while(bundleLenRemaining > 0)
		{
			bytesToRead = MIN(bundleLenRemaining, sizeof(buffer)-1);
			rc = zco_receive_source(sdr, &reader, bytesToRead,
					buffer);
			if(rc < 0) break;
			bundleLenRemaining -= rc;
			printf("%.*s", rc, buffer);
			fflush(stdout);
		}

		if (sdr_end_xn(sdr) < 0)
		{
			running = 0;
		}

		pthread_mutex_unlock(&sdrmutex);
		bp_release_delivery(&dlv, 1);
	}        
	return NULL;
}

void handleQuit(int sig)
{
	running = 0;
	pthread_end(sendLinesThread);
	bp_interrupt(sap);
}

int main(int argc, char **argv)
{
	ownEid      = (argc > 1 ? argv[1] : NULL);
	destEid     = (argc > 2 ? argv[2] : NULL);
	char    *ctArg = (argc > 3 ? argv[3] : NULL);

	if(argc < 2 || (argv[1][0] == '-')) {
		fprintf(stderr, usage);
		exit(1);
	}

	if(ctArg && strncmp(ctArg, "ct", 3) == 0) {
		custodySwitch = SourceCustodyRequired;
	}

	if(bp_attach() < 0) {
		putErrmsg("Can't bp_attach()", NULL);
		exit(1);
	}

	if(bp_open(ownEid, &sap) < 0) 
	{
		putErrmsg("Can't open own endpoint.", ownEid);
		exit(1);
	}

	sdr = bp_get_sdr();

	signal(SIGINT, handleQuit);

	/* Start receiver thread and sender thread. */
	if(pthread_begin(&sendLinesThread, NULL, sendLines, NULL) < 0) {
		putErrmsg("Can't make sendLines thread.", NULL);
		bp_interrupt(sap);
		exit(1);
	}

	if(pthread_begin(&recvBundlesThread, NULL, recvBundles, NULL) < 0) {
		putErrmsg("Can't make recvBundles thread.", NULL);
		bp_interrupt(sap);
		exit(1);
	}

	pthread_join(sendLinesThread, NULL);
	pthread_join(recvBundlesThread, NULL);

	bp_close(sap);
	bp_detach();
	return 0;
}
