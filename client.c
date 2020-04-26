#include <SDL.h>
#include <assert.h>
#include <curses.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h> // memset için
#include <unistd.h> // close için
#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>

/* constants *****************************************************************/

#define SUNUCUPORT 2015
#define SUNUCUIP "192.168.1.100"
#define AZAMIUZUNLUK 1024
#define VERSION "0.1"
#define HELP_MESSAGE "nope"

/* functions *****************************************************************/

long map(long x, long in_min, long in_max, long out_min, long out_max)
{
    if (x == in_max)
        return out_max;
    else if (out_min < out_max)
        return (x - in_min) * (out_max - out_min + 1) / (in_max - in_min) + out_min;
    else
        return (x - in_min) * (out_max - out_min - 1) / (in_max - in_min) + out_min;
}

int str2int(const char *str, int *val)
{
    char *endptr;
    errno = 0;

    *val = strtol(str, &endptr, 10);

    /* hatalr için denetim */
    if ((errno == ERANGE && (*val == LONG_MAX || *val == LONG_MIN)) || (errno != 0 && *val == 0))
    {
        return 0;
    }

    if (endptr == str)
    {
        return 0;
    }

    return 1;
}

void print_joystick_info(int joy_idx, SDL_Joystick *joy, SDL_GameController *gamepad)
{
    SDL_JoystickGUID guid = SDL_JoystickGetGUID(joy);
    char guid_str[1024];
    SDL_JoystickGetGUIDString(guid, guid_str, sizeof(guid_str));

    printf("Joystick Name:     '%s'\n", SDL_JoystickName(joy));
    printf("Joystick GUID:     %s\n", guid_str);
    printf("Joystick Number:   %2d\n", joy_idx);
    printf("Number of Axes:    %2d\n", SDL_JoystickNumAxes(joy));
    printf("Number of Buttons: %2d\n", SDL_JoystickNumButtons(joy));
    printf("Number of Hats:    %2d\n", SDL_JoystickNumHats(joy));
    printf("Number of Balls:   %2d\n", SDL_JoystickNumBalls(joy));
    printf("GameController:\n");
    if (!gamepad)
    {
        printf("  not a gamepad\n");
    }
    else
    {
        printf("  Name:    '%s'\n", SDL_GameControllerName(gamepad));
        printf("  Mapping: '%s'\n", SDL_GameControllerMappingForGUID(guid));
    }
    printf("\n");
}

struct senderd
{
    int32_t ileri_geri;
    int32_t sag_sol;
    int32_t dal_cik;
    int32_t button_number[12];
};

struct senderd sende;

void send_print(struct senderd *p)
{
    for (int i = 0; i <= 11; i++)
    {
        printf("button_number_%d : %d \n", i, p->button_number[i]);
    }
    printf("\n");
}


/* main() ********************************************************************/
int main(int argc, char **argv)
{

    int sockfd;
    struct sockaddr_in serverAddr;
    char *gonder = "Kaynak hangi site?";
    char str[AZAMIUZUNLUK];
    int gelenBayt, gidenBayt, structSize;
    
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == sockfd)
    {
        perror("socket");
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SUNUCUPORT);
    serverAddr.sin_addr.s_addr = inet_addr(SUNUCUIP);
    memset(&(serverAddr.sin_zero), '\0', 8);

    if (-1 == connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr)))
    {
        perror("connect");
    }

    int joy_idx = 0;
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");

    // FIXME: We don't need video, but without it SDL will fail to work in SDL_WaitEvent()
    if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC) < 0)
    {
        fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
        exit(1);
    }
    else
    {
        atexit(SDL_Quit);
        SDL_Joystick *joy = SDL_JoystickOpen(joy_idx);
        if (!joy)
        {
            fprintf(stderr, "Unable to open joystick %d\n", joy_idx);
        }
        else
        {
            print_joystick_info(joy_idx, joy, NULL);

            printf("Entering joystick test loop, press Ctrl-c to exit\n");
            int quit = 0;
            SDL_Event event;

            while (!quit && SDL_WaitEvent(&event))
            {
                switch (event.type)
                {
                case SDL_JOYAXISMOTION:
                    //printf("SDL_JOYAXISMOTION: joystick: %d axis: %d value: %d\n",event.jaxis.which, event.jaxis.axis, event.jaxis.value);
                    if (event.jaxis.axis == 1)
                    {
                        printf("ileri_geri: joystick: %d axis: %d value: %d\n", event.jaxis.which, event.jaxis.axis, event.jaxis.value * -1);
                        if (event.jaxis.value > 129 || event.jaxis.value < -129)
                        {
                            sende.ileri_geri = event.jaxis.value * -1;
                        }
                        else
                        {
                            sende.ileri_geri = 0;
                        }
                        printf("%d\n", sende.ileri_geri);
                    }
                    else if (event.jaxis.axis == 0)
                    {
                        printf("sag_sol: joystick: %d axis: %d value: %d\n", event.jaxis.which, event.jaxis.axis, event.jaxis.value * -1);
                        if (event.jaxis.value > 129 || event.jaxis.value < -129)
                        {
                            sende.sag_sol = event.jaxis.value * -1 ;
                        }
                        else
                        {
                            sende.sag_sol = 0;
                        }
                        printf("%d\n", sende.sag_sol);
                    }
                    else if (event.jaxis.axis == 3)
                    {
                        printf("dal_cik: joystick: %d axis: %d value: %d\n", event.jaxis.which, event.jaxis.axis, event.jaxis.value * -1);
                        if (event.jaxis.value > 129 || event.jaxis.value < -129)
                        {
                            sende.dal_cik = event.jaxis.value * -1;
                        }
                        else
                        {
                            sende.dal_cik = 0;
                        }
                        printf("%d\n", sende.dal_cik);
                    }
                    break;

                case SDL_JOYBUTTONDOWN:
                    //printf("SDL_JOYBUTTONDOWN: joystick: %d button: %d state: %d\n",event.jbutton.which, event.jbutton.button, event.jbutton.state);
                    if (event.jbutton.button == 0)
                    {
                        sende.button_number[0] = 1;
                    }
                    else if (event.jbutton.button == 1)
                    {
                        sende.button_number[1] = 1;
                    }
                    else if (event.jbutton.button == 2)
                    {
                        sende.button_number[2] = 1;
                    }
                    else if (event.jbutton.button == 3)
                    {
                        sende.button_number[3] = 1;
                    }
                    else if (event.jbutton.button == 4)
                    {
                        sende.button_number[4] = 1;
                    }
                    else if (event.jbutton.button == 5)
                    {
                        sende.button_number[5] = 1;
                    }
                    else if (event.jbutton.button == 6)
                    {
                        sende.button_number[6] = 1;
                    }
                    else if (event.jbutton.button == 7)
                    {
                        sende.button_number[7] = 1;
                    }
                    else if (event.jbutton.button == 8)
                    {
                        sende.button_number[8] = 1;
                    }
                    else if (event.jbutton.button == 9)
                    {
                        sende.button_number[9] = 1;
                    }
                    else if (event.jbutton.button == 10)
                    {
                        sende.button_number[10] = 1;
                    }
                    else if (event.jbutton.button == 11)
                    {
                        sende.button_number[11] = 1;
                    }
                    else
                    {
                        for (int i = 0; i <= 11; i++)
                        {
                            sende.button_number[i] = 0;
                        }
                    }
                    send_print(&sende);
                    break;

                case SDL_JOYBUTTONUP:
                    //printf("SDL_JOYBUTTONUP: joystick: %d button: %d state: %d\n",event.jbutton.which, event.jbutton.button, event.jbutton.state);

                    for (int i = 0; i <= 11; i++)
                    {
                        sende.button_number[i] = 0;
                    }
                    send_print(&sende);

                    break;

                case SDL_JOYHATMOTION:
                    printf("SDL_JOYHATMOTION: joystick: %d hat: %d value: %d\n",
                           event.jhat.which, event.jhat.hat, event.jhat.value);
                    break;

                case SDL_JOYBALLMOTION:
                    printf("SDL_JOYBALLMOTION: joystick: %d ball: %d x: %d y: %d\n",
                           event.jball.which, event.jball.ball, event.jball.xrel, event.jball.yrel);
                    break;

                case SDL_JOYDEVICEADDED:
                    printf("SDL_JOYDEVICEADDED which:%d\n", event.jdevice.which);
                    break;

                case SDL_JOYDEVICEREMOVED:
                    printf("SDL_JOYDEVICEREMOVED which:%d\n", event.jdevice.which);
                    break;

                case SDL_CONTROLLERBUTTONDOWN:
                    printf("SDL_CONTROLLERBUTTONDOWN\n");
                    break;

                case SDL_CONTROLLERBUTTONUP:
                    printf("SDL_CONTROLLERBUTTONUP\n");
                    break;

                case SDL_CONTROLLERDEVICEADDED:
                    printf("SDL_CONTROLLERDEVICEADDED which:%d\n", event.cdevice.which);
                    break;

                case SDL_CONTROLLERDEVICEREMOVED:
                    printf("SDL_CONTROLLERDEVICEREMOVED which:%d\n", event.cdevice.which);
                    break;

                case SDL_CONTROLLERDEVICEREMAPPED:
                    printf("SDL_CONTROLLERDEVICEREMAPPED which:%d\n", event.cdevice.which);
                    break;

                case SDL_QUIT:
                    quit = 1;
                    printf("Recieved interrupt, exiting\n");
                    break;

                default:
                    fprintf(stderr, "Error: Unhandled event type: %d\n", event.type);
                    break;
                }
                gidenBayt = send(sockfd, &sende, sizeof(sende), 0);
                if (-1 == gidenBayt)
                {
                    perror("send");
                }
            }
            SDL_JoystickClose(joy);
            close(sockfd);
        }
    }
}

/* EOF */
