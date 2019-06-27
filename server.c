#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include "difdrive.h"
#include <pigpio.h>
#include <stdlib.h>
#include <signal.h>

#define LISTENPORT 2015

#define LSBFIRST 0
#define MSBFIRST 1

#define DATA_PIN 22
#define LATCH_PIN 27
#define CLOCK_PIN 17

#define CONNECTED 57//uint8_t c  = 0b00111001; // 57 connected  00111001
#define NOCLIENT 55 //uint8_t nc = 0b00110111; // 55 no connect 00110111
#define CLOSED 63   //uint8_t cl = 0b00111111; // 63 closed     00111111
#define ERROR 121   //uint8_t er = 0b01111001; // 121 error     01111001
#define KILL 143    //uint8_t k  = 0b10001111; // 143 kill      10001111
#define ACIK 119

struct senderd
{
    int32_t ileri_geri;
    int32_t sag_sol;
    int32_t dal_cik;
    int32_t button_number[12];
};

struct senderd sende;

void shiftOut(uint8_t bitOrder, uint8_t val)
{
     uint8_t i;
     gpioWrite(LATCH_PIN,0);
     for (i = 0; i < 8; i++)  {
           if (bitOrder == LSBFIRST)
                 gpioWrite(DATA_PIN, !!(val & (1 << i)));
           else
                 gpioWrite(DATA_PIN, !!(val & (1 << (7 - i))));

           gpioWrite(CLOCK_PIN, 1);
           gpioWrite(CLOCK_PIN, 0);
     }
     gpioWrite(LATCH_PIN,1);
}

void exitHandle(){
    if (gpioInitialise() < 0) return -1;
    clear();
    shiftOut(MSBFIRST,KILL);
    clear();
    gpioTerminate();
}

void send_print(struct senderd *p)
{
    FILE * serverdata;
    serverdata = fopen("/tmp/server.txt","w");
    for (int i = 0; i <= 11; i++)
    {
        printf("button_number_%d : %d \n", i, p->button_number[i]);
        fprintf(serverdata,"button_number_%d : %d \n", i, p->button_number[i]);
    }
    printf("ileri_geri : %d \n", map(p->ileri_geri, -32767, 32768, -400, 400));
    fprintf(serverdata,"ileri_geri : %d \n", map(p->ileri_geri, -32767, 32768, -400, 400));
    printf("sag_sol : %d \n", map(p->sag_sol, -32767, 32768, -400, 400));
    fprintf(serverdata,"sag_sol : %d \n", map(p->sag_sol, -32767, 32768, -400, 400));
    CalculateTankDrive(p->sag_sol,p->ileri_geri);
    printf("dal_cik : %d \n", 1500 + map(p->dal_cik, -32767, 32767, -400, 400));
    fprintf(serverdata,"dal_cik : %d \n", 1500 + map(p->dal_cik, -32767, 32767, -400, 400));
    gpioServo(DALIS_SAG, 1500 + map(p->dal_cik, -32767, 32767, -400, 400));
    gpioServo(DALIS_SOL, 1500 + map(p->dal_cik, -32767, 32767, -400, 400));
    printf("\n");
    fprintf(serverdata,"\n");
    fclose(serverdata);
}

void clear(){
	shiftOut(MSBFIRST,0);
}

void signalHandler(){
	exitHandle();
	exit(EXIT_FAILURE);
}

int main()
{
    if (gpioInitialise() < 0) return -1;
    gpioSetMode(17, PI_OUTPUT); //Select
    gpioSetMode(27, PI_OUTPUT); //Latch
    gpioSetMode(22, PI_OUTPUT); //Data
    signal(SIGTERM, signalHandler);
    atexit(exitHandle);
    clear();
    shiftOut(MSBFIRST,ACIK);
//    shiftOut(MSBFIRST,ACIK);

    int sockfd, newfd;
    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;
    int gelenBayt, gidenBayt, structSize;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == sockfd)
    {
        perror("socket");
	clear();
	shiftOut(MSBFIRST,ERROR);
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(LISTENPORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    memset(&(serverAddr.sin_zero), '\0', 8);

    if (-1 == bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr)))
    {
        perror("bind");
	clear();
	shiftOut(MSBFIRST,ERROR);
    }

    if (-1 == listen(sockfd, 20))
    {
        perror("listen");
	clear();
	shiftOut(MSBFIRST,ERROR);
    }

    structSize = sizeof(clientAddr);
    newfd = accept(sockfd, (struct sockaddr *)&clientAddr, &structSize);
    if (-1 == newfd)
    {
        perror("accept");
	clear();
	shiftOut(MSBFIRST,ERROR);
    }else{
	clear();
        shiftOut(MSBFIRST,CONNECTED);
	}
	
    while (1)
    {
        gelenBayt = recv(newfd, &sende, sizeof(sende), 0);
        if (-1 == gelenBayt)
        {
            perror("recv");
        }
        else if (0 == gelenBayt)
        {
	    if (gpioInitialise() < 0) return -1;
	    clear();
	    shiftOut(MSBFIRST,63);
            printf("Bağlantı kapalı.\n");
        }
        //printf("a : %d b: %d c: %d \n", sende.ileri_geri, sende.button_number[1], sende.dal_cik);
        send_print(&sende);
	gpioServo(R_OUT_FRONT, 1500 + RightMotorOutput);
	gpioServo(R_OUT_BACK,  1500 + RightMotorOutput);
	gpioServo(L_OUT_FRONT, 1500 + LeftMotorOutput);
	gpioServo(L_OUT_BACK, 1500 + LeftMotorOutput);
        /* gidenBayt = send(newfd, gonder, strlen(gonder), 0);
	if (-1 == gidenBayt) {  
		perror("send");
	}
	else if (strlen(gonder) != gidenBayt) {
		printf("Gönderilen: %d\tGiden: %d\n", strlen(gonder), gidenBayt);
	}
	printf("%d bayt gönderdim:\t%s\n", gidenBayt, gonder); */
    }
    close(newfd);
    close(sockfd);
//    gpioTerminate();

    //int exitf = atexit(exitHandler);
    //if (exitf != 0) {
    //            fprintf(stderr, "cannot set exit function\n");
    //            exit(EXIT_FAILURE);
    // }
    // exit(EXIT_SUCCESS);
    return 0;
}
