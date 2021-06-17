#include "mbed.h"
#include "bbcar.h"
#include "mbed_rpc.h"
// #include "bbcar_rpc.h"
#include <cmath>

const int PI = 3.141592654;

Ticker servo_ticker;
PwmOut pin5(D5), pin6(D6);
BBCar car(pin5, pin6, servo_ticker);

BufferedSerial pc(USBTX,USBRX); //tx,rx                                                                                                                                 
BufferedSerial uart(D1,D0); //tx,rx                                                                                                                                     

DigitalInOut ping(D11);
Timer t;

Thread thread;
float ping_distance;

/////////////////////////////////////////////////////////////////////////////////////////
// Encoder related
/* #region Encoder / Run */
Ticker encoder_ticker;
DigitalIn encoder_left(D12);
DigitalIn encoder_right(D13);
volatile int steps_left,steps_right;
volatile int last_left,last_right;

void encoder_control() {
   int value_left = encoder_left;
   int value_right = encoder_right;
   if (!last_left && value_left) steps_left++;
   if (!last_right && value_right) steps_right++;
   last_left = value_left;
   last_right = value_right;
}
void go_straight_by_distance(int distance) {
   encoder_ticker.attach(&encoder_control, 10ms);
   steps_left = 0;
   steps_right = 0;
   last_left = 0;
   last_right = 0;

   car.goStraight(100);
   if(distance>9) distance-5;
   while(steps_right*6.5*PI/32 < distance) {
      ThisThread::sleep_for(100ms);
   }
   car.stop();
}
void reverse_by_distance(int distance) {
   encoder_ticker.attach(&encoder_control, 10ms);
   steps_left = 0;
   steps_right = 0;
   last_left = 0;
   last_right = 0;

   car.goStraight(-100);
   while(steps_right*6.5*PI/32 < distance-9) {
      ThisThread::sleep_for(100ms);
   }
   car.stop();
}
void turn_by_angle(int angle){
   encoder_ticker.attach(&encoder_control, 5ms);
   int steps_req = abs(angle/90*28 - angle/15);
   if(angle==90) steps_req -4;
   steps_left = 0;
   steps_right = 0;
   last_left = 0;
   last_right = 0;
   // angle > 0, turn right
   if (angle>0){
      car.turn(30,-0.001);
      // while(steps_left*6.5*PI/32 < 15) {
      while(steps_left <= steps_req) {
         // printf("encoder = %d\r\n", steps_left);
         ThisThread::sleep_for(30ms);
      }
   }
   // angle < 0, turn left
   else if (angle<0){
      car.turn(30,0.001);
      while(steps_right <= steps_req) {
      // while(steps_right*6.5*PI/32 < 15) {
         // printf("encoder = %d\r\n", steps);
         ThisThread::sleep_for(30ms);
      }
   }
   car.stop();
}
void reverse_turn_by_angle(int angle){
   encoder_ticker.attach(&encoder_control, 5ms);
   int steps_req = angle/90*28 - angle/14 + 2;
   steps_left = 0;
   steps_right = 0;
   last_left = 0;
   last_right = 0;
   // angle > 0, turn right
   if (angle>0){
      car.turn(-30,-0.001);
      // while(steps_left*6.5*PI/32 < 15) {
      while(steps_left <= steps_req) {
         // printf("encoder = %d\r\n", steps_left);
         ThisThread::sleep_for(30ms);
      }
   }
   // angle < 0, turn left
   else if (angle<0){
      car.turn(-30,0.001);
      while(steps_right <= -steps_req) {
      // while(steps_right*6.5*PI/32 < 15) {
         // printf("encoder = %d\r\n", steps);
         ThisThread::sleep_for(30ms);
      }
   }
   car.stop();
}
/* #endregion */
/////////////////////////////////////////////////////////////////////////////////////////c
// RPC related
/* #region RPC */

/* #region original RPC */
void RPC_stop (Arguments *in, Reply *out)   {
    car.stop();
    return;
}
void RPC_goStraight (Arguments *in, Reply *out)   {
    int speed = in->getArg<double>();
    car.goStraight(100);
    return;
}
void RPC_turn (Arguments *in, Reply *out)   {
    int speed = in->getArg<double>();
    double turn = in->getArg<double>();
    car.turn(speed,turn);
    return;
}

RPCFunction rpcStop(&RPC_stop, "stop");
RPCFunction rpcCtrl(&RPC_goStraight, "goStraight");
RPCFunction rpcTurn(&RPC_turn, "turn");
/* #endregion */

void RPC_goStraightByDistance (Arguments *in, Reply *out)   {
   int distance = in->getArg<double>();
   go_straight_by_distance(distance);
   return;
}
void RPC_turnAngle (Arguments *in, Reply *out)   {
   int angle = in->getArg<double>();
   turn_by_angle(angle);
   return;
}
void RPC_turnAngleRev (Arguments *in, Reply *out)   {
   int angle = in->getArg<double>();
   reverse_turn_by_angle(angle);
   return;
}
void RPC_analyse (Arguments *in, Reply *out) {
   int d1 = in->getArg<double>();
      d1=d1+7;
   int d2 = in->getArg<double>();
   int direction = in->getArg<double>();
   if (direction == 0){
      go_straight_by_distance(d2);
      ThisThread::sleep_for(2s);
      reverse_turn_by_angle(-90);
      ThisThread::sleep_for(2s);
      go_straight_by_distance(d1);
   }
   if (direction == 1){
      go_straight_by_distance(d2);
      ThisThread::sleep_for(2s);
      reverse_turn_by_angle(90);
      ThisThread::sleep_for(2s);
      go_straight_by_distance(d1);
   }
}

void RPC_aprilTag (Arguments *in, Reply *out)   {
    float degree = in->getArg<double>();
    float distance = in->getArg<double>();
    if(degree<=180){
        float x_distance = distance * sin(degree);
        float y_distance = distance * cos(degree) -5;
        int degree_to_turn = 90-degree+13;
        printf("turning 1\n");
        turn_by_angle(degree_to_turn);
        ThisThread::sleep_for(2s);
        printf("straight 1\n");
      //   go_straight_by_distance(x_distance);
         car.goStraight(100);
         ThisThread::sleep_for(1s);
         car.stop();
         ThisThread::sleep_for(2s);
         printf("turning 2\n");
         turn_by_angle(-90);
         ThisThread::sleep_for(2s);
         printf("straight 2\n");
         car.goStraight(100);
         printf("%dcm",ping_distance*17700.4f-4);
         while(ping_distance*17700.4f-4>25){
            printf("%dcm",ping_distance);
            ThisThread::sleep_for(100ms);
         }
         car.stop();
         printf("end\n");

      //   go_straight_by_distance(y_distance);
        // ping
    }
    else {
        float pho = 360 - degree;
        float init_turn_degree = -(90-pho);
        float x_distance = distance * sin(degree);
        float y_distance = distance * cos(degree) -5;
        turn_by_angle(init_turn_degree-5);
        ThisThread::sleep_for(2s);
        go_straight_by_distance(x_distance);
        ThisThread::sleep_for(2s);
        turn_by_angle(90);
        ThisThread::sleep_for(2s);
        go_straight_by_distance(y_distance);
        // ping
    }
    return;
}

RPCFunction rpcRunDist(&RPC_goStraightByDistance, "goDistance");
RPCFunction rpcTurnByAngle(&RPC_turnAngle, "turnAngle");
RPCFunction rpcRevTurnByAngle(&RPC_turnAngleRev, "turnAngleRev");
RPCFunction rpcAnalyse(&RPC_analyse, "parking");
RPCFunction rpcAprilTag(&RPC_aprilTag, "apriltag_calibration");
/* #endregion */

void rpc() {
   char buf[256], outbuf[256];
   FILE *devin = fdopen(&uart, "r");
   FILE *devout = fdopen(&uart, "w");
   uart.set_baud(9600);
   while (1) {
      memset(buf, 0, 256);
      for( int i = 0; ; i++ ) {
         char recv = fgetc(devin);
         if(recv == '\n') {
            printf("\r\n");
            break;
         }
         buf[i] = fputc(recv, devout);
      }
   RPC::call(buf, outbuf);
   }

}

int main(){
    thread.start(rpc);

   //  /* #region PING */

   //  pc.set_baud(9600);
   //  while(1){

   //      ping.output();
   //      ping = 0; wait_us(200);
   //      ping = 1; wait_us(5);
   //      ping = 0; wait_us(5);

   //      ping.input();
   //      while(ping.read() == 0);
   //      t.start();
   //      while(ping.read() == 1);
   //      ping_distance = t.read();
   //      printf("Distance = %lf cm\r\n", ping_distance*17700.4f-4);
   //      t.stop();
   //      t.reset();

   //      ThisThread::sleep_for(1s);

   //  }
   //  /* #endregion */
}