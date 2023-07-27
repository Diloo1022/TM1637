#include <stdio.h>
#include <stdlib.h>	//system
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/ioctl.h>
#include <pthread.h>
#include <time.h>


// IOCTL
#define tm_IOC_MAGIC 'M'
#define DISPLAY_NUMBER _IOW(tm_IOC_MAGIC,0,int)
#define SET_BRIGHTNESS _IOW(tm_IOC_MAGIC,1,int)
#define SHOW_DOT _IOW(tm_IOC_MAGIC,2,int)
#define ON_OFF _IOW(tm_IOC_MAGIC,3,int)





#define DEVICE_BLTEST "/dev/tm1637"
int choise=0;
int tmfd;



void* stopWatch(void* arg) {
	int num = 0;
	
	sleep(2);
	printf("\nClock tickling!\n");
	
	while(choise != 11){
		ioctl(tmfd, DISPLAY_NUMBER, num);
		if(num == 59) num+= 41;
		else                num++;
		
		
		sleep(1);
		}
	pthread_exit(NULL); // 離開子執行緒
}

void* displayClock(void* arg) {
	time_t timer ;
	struct tm * Now ;
	int hour,minute,num,i=0,on=0;

	time ( & timer ) ;
	Now = localtime ( & timer ) ;
	hour = Now->tm_hour;
	minute = Now->tm_min;
	num = hour*100 + minute;
	
	printf("\nClock displaying!");
	printf("\nPresent time : %s\n\n",asctime ( Now ));
	
	while(choise != 11){
		
		if(i == 59) {	num++;		i = 0;	}
		else i++;
		ioctl(tmfd, DISPLAY_NUMBER, num);

		on = on? 0 : 1;
		ioctl(tmfd, SHOW_DOT, on);
		sleep(1);
		}
	pthread_exit(NULL); // 離開子執行緒
}


int main(void)
{
	pthread_t t; // 宣告 pthread 變數
	int  i;
	int num;
	int ret = 0;
	char buf[5];
	//***************************
	
	
	//******************************
	
	printf("process start!\n\n");
	sleep(1);
	
	tmfd = open(DEVICE_BLTEST, O_RDONLY);
	if(tmfd < 0)
	{
		perror("Open device failure\n");
		exit(1);
	}
	
	
	
	while(choise != 7){
	printf("\nWhat do you want to do?\n(1)Change display number\n(2)Set brightness\n(3)Set dot display\n(4)Count down\n(5)Stop watch\n(6)Clock\n(7)Exit\n");
	scanf("%d",&choise);
	
	switch(choise){
		case 1:
			printf("Input display number(0000~9999):");
			scanf("%d",&num);
			ioctl(tmfd, DISPLAY_NUMBER, num);
			break;
		case 2:
			printf("Input brightness(0~7):");
			scanf("%d",&num);
			ioctl(tmfd, SET_BRIGHTNESS, num);
			break;
		case 3:
			printf("Showing dot?(0~1):");
			scanf("%d",&num);
			ioctl(tmfd, SHOW_DOT, num);
			break;
		case 4:
			printf("Count down from seconds(0~9999) : ");
			scanf("%d",&num);

			while(num>=0){
				ioctl(tmfd, DISPLAY_NUMBER, num);
				num--;
				sleep(1);
				}
			
			sleep(2);
			printf("Count down completed!\n ");
			break;

		case 5:
			printf("Start stop watch function\n ");
			pthread_create(&t, NULL, stopWatch, NULL);
			while(choise!=11){
				printf("Input 11 if you wanna stop\n");
				scanf("%d",&choise);
				}
			pthread_join(t, NULL);
			printf("Stop watch completed!\n ");
			for(i=0;i<3;i++){
				ioctl(tmfd, ON_OFF, 0);
				sleep(1);
				ioctl(tmfd, ON_OFF, 1);
				sleep(1);
				}		
			break;
		case 6:
			printf("Start display clock function\n ");
			pthread_create(&t, NULL, displayClock, NULL);
			while(choise!=11){
				printf("Input 11 if you wanna stop\n");
				scanf("%d",&choise);
				}
			pthread_join(t, NULL);
			printf("Clock display finished!\n ");
		
			break;
		default:
			if(choise != 7)
				printf("Please input effective number (1~4)\n");
			break;
		}

	}
	
	if (tmfd >= 0)	 //close tmfd if open
	{
		close(tmfd);
	}
	
	return 0;
}
