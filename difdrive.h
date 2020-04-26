#include <math.h>
#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

#define DDRIVE_MIN -32767 //joystickten gelen en düşük değer x - y vektörleri
#define DDRIVE_MAX 32768  //joystickten gelen en büyük değer x - y vektörleri
#define MOTOR_MIN_PWM -400 //motor hızı için en düşük pwm değeri
#define MOTOR_MAX_PWM 400 //motor hızı için en yüksek pwm değeri

//pin tanımları
#define R_OUT_FRONT
#define R_OUT_BACK
#define L_OUT_FRONT
#define L_OUT_BACK

int LeftMotorOutput; //sol motor çıkış değeri
int RightMotorOutput; //sağ motor çıkış değeri

long map(long x, long in_min, long in_max, long out_min, long out_max)
{
    if (x == in_max)
        return out_max;
    else if (out_min < out_max)
        return (x - in_min) * (out_max - out_min + 1) / (in_max - in_min) + out_min;
    else
        return (x - in_min) * (out_max - out_min - 1) / (in_max - in_min) + out_min;
}
 
void CalculateTankDrive(float xe, float ye) //ex = x ekseni değeri, ye= y ekseni değeri 
{

    float x = xe * -1;
    float y = ye ;
    float rawLeft;
    float rawRight;
    float RawLeft;
    float RawRight;
    
    float z = sqrt(x * x + y * y);

    float rad = acos(abs(x) / z);

    if (isnan(rad) == 1) {
        rad = 0;
    }
    
    float angle = rad * 180 / M_PI;

    float tcoeff = -1 + (angle / 90) * 2;
    float turn = tcoeff * abs(abs(y) - abs(x));
    turn = round(turn * 100) / 100;

    float mov = max(abs(y), abs(x));

    if ((x >= 0 && y >= 0) || (x < 0 && y < 0))
    {
        rawLeft = mov; rawRight = turn;
    }
    else
    {
        rawRight = mov; rawLeft = turn;
    }

    if (y < 0) {
        rawLeft = 0 - rawLeft;
        rawRight = 0 - rawRight;
    }

    RawLeft = rawLeft;
    RawRight = rawRight;

    LeftMotorOutput = map(rawLeft, DDRIVE_MIN, DDRIVE_MAX, MOTOR_MIN_PWM, MOTOR_MAX_PWM);
    RightMotorOutput = map(rawRight, DDRIVE_MIN, DDRIVE_MAX, MOTOR_MIN_PWM, MOTOR_MAX_PWM);
 
 
    printf("LeftMotorOutput  : %d\n",LeftMotorOutput);
    printf("RightMotorOutput : %d\n",RightMotorOutput);


}
