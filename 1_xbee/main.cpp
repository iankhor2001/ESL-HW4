#include "mbed.h"
#include "bbcar.h"
#include "mbed_rpc.h"
// #include "bbcar_rpc.h"

const int PI = 3.141592654;

Ticker servo_ticker;
PwmOut pin5(D5), pin6(D6);
BufferedSerial xbee(D1, D0);
BBCar car(pin5, pin6, servo_ticker);

/////////////////////////////////////////////////////////////////////////////////////////
// Encoder related

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
   xbee.set_baud(9600);
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
   int steps_req = angle/90*28 - angle/15;
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
      while(steps_right <= -steps_req) {
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

/////////////////////////////////////////////////////////////////////////////////////////c
// RPC related
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

RPCFunction rpcRunDist(&RPC_goStraightByDistance, "goDistance");
RPCFunction rpcTurnByAngle(&RPC_turnAngle, "turnAngle");
RPCFunction rpcRevTurnByAngle(&RPC_turnAngleRev, "turnAngleRev");
RPCFunction rpcAnalyse(&RPC_analyse, "parking");


int main() {
   char buf[256], outbuf[256];
   FILE *devin = fdopen(&xbee, "r");
   FILE *devout = fdopen(&xbee, "w");
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