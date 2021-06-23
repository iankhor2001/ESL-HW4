#include "mbed.h"
#include "bbcar.h"
#include "mbed_rpc.h"
#include <stdio.h>
// #include "bbcar_rpc.h"

const int PI = 3.141592654;

Ticker servo_ticker;
PwmOut pin5(D5), pin6(D6);
BufferedSerial xbee(D1, D0);
BBCar car(pin5, pin6, servo_ticker);
DigitalIn mypin(USER_BUTTON);
BufferedSerial uart(D10,D9);

DigitalInOut ping(D11);
Timer t;
float ping_distance;

Thread xbee_thread,uart_thread;

/////////////////////////////////////////////////////////////////////////////////////////
// Encoder related

Ticker encoder_ticker;
DigitalIn encoder_left(D12);
DigitalIn encoder_right(D13);
volatile int steps_left,steps_right;
volatile int last_left,last_right;
void stoping_openmv();

/* #region functions */
float translate_theo_to_actual_distance(float distance){
    // apparently the function isn't linear with the actual distance, 
    // thus i hv to use a polynomial function to get the difference through trial and error and experimentation
    // y = -0.000013x^3 - 0.002959x^2 + 0.956015x - 4.067411
    float distance_code = 0.956*distance -4.067 -2.959/1000*distance*distance - 13/100000*distance*distance*distance;
    return distance_code;
}
float translate_theo_to_actual_right_angle(float angle){
    // y = 0.001162x2 + 0.744048x - 1.403823
    float angle_code = 0.744*angle -1.404 +1.16/1000*angle*angle;
    return angle_code;
}
void encoder_control() {
   int value_left = encoder_left;
   int value_right = encoder_right;
   if (!last_left && value_left) steps_left++;
   if (!last_right && value_right) steps_right++;
   last_left = value_left;
   last_right = value_right;
}
void go_straight_by_distance(int distance) {
    float distance_code = translate_theo_to_actual_distance(distance);
    xbee.set_baud(9600);
    encoder_ticker.attach(&encoder_control, 10ms);
    steps_left = 0;
    steps_right = 0;
    last_left = 0;
    last_right = 0;

    car.goStraight(100);
    while(steps_right*6.5*PI/32 < distance_code) {
        ThisThread::sleep_for(100ms);
    }
    car.stop();
}
void turn_by_angle(int angle){
    encoder_ticker.attach(&encoder_control, 5ms);
    float angle_code = translate_theo_to_actual_right_angle(abs(angle));
    float distance = abs(angle_code)*(18.064/90);
    // printf("distance=%.2f,code=%.2f\r\n",distance,distance_code);
    steps_left = 0;
    steps_right = 0;
    last_left = 0;
    last_right = 0;
    // angle > 0, turn right
    if (angle>0){
        car.turn(100,-0.001);
        // while(steps_left*6.5*PI/32 < 15) {
        while(steps_left*6.5*PI/32 <= distance) {
            // printf("encoder = %d\r\n", steps_left);
            ThisThread::sleep_for(30ms);
        }
    }
    // angle < 0, turn left
    else if (angle<0){
        car.turn(100,0.001);
        while(steps_right*6.5*PI/32 <= distance) {
        // while(steps_right*6.5*PI/32 < 15) {
            // printf("encoder = %d\r\n", steps);
            ThisThread::sleep_for(30ms);
        }
    }
    car.stop();
}
void reverse_turn_by_angle(int angle){
    encoder_ticker.attach(&encoder_control, 5ms);
    float angle_code = translate_theo_to_actual_right_angle(abs(angle));
    float distance = (angle_code)*(18.064/90);
    // printf("distance=%.2f,code=%.2f\r\n",distance,distance_code);
    steps_left = 0;
    steps_right = 0;
    last_left = 0;
    last_right = 0;
    // angle > 0, turn right
    if (angle>0){
        car.turn(-100,0.001);
        // while(steps_left*6.5*PI/32 < 15) {
        while(steps_right*6.5*PI/32 <= distance) {
            // printf("encoder = %d\r\n", steps_left);
            ThisThread::sleep_for(30ms);
        }
    }
    // angle < 0, turn left
    else if (angle<0){
        car.turn(-100,-0.001);
        while(steps_left*6.5*PI/32 <= distance) {
        // while(steps_right*6.5*PI/32 < 15) {
            // printf("encoder = %d\r\n", steps);
            ThisThread::sleep_for(30ms);
        }
    }
    car.stop();
}
/* #endregion functions */
/////////////////////////////////////////////////////////////////////////////////////////c
// RPC related

/* #region original RPC */
void RPC_stop (Arguments *in, Reply *out)   {
    car.stop();
    printf("RPC Stop");
    return;
}
void RPC_goStraight (Arguments *in, Reply *out)   {
    int speed = in->getArg<double>();
    printf("RPC Go straight &.2f\n",speed);
    car.goStraight(100);
    return;
}
void RPC_turn (Arguments *in, Reply *out)   {
    int speed = in->getArg<double>();
    double turn = in->getArg<double>();
    printf("RPC Turn &.2f\n",turn);
    car.turn(speed,turn);
    return;
}

RPCFunction rpcStop(&RPC_stop, "stop");
RPCFunction rpcCtrl(&RPC_goStraight, "goStraight");
RPCFunction rpcTurn(&RPC_turn, "turn");
/* #endregion */

/* #region my RPC */
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
    xbee.write("Starting AprilTag Calibration Segment\r\n",38);
    float degree = in->getArg<double>();
    float distance = in->getArg<double>();
    if(degree<=180){
        float y_distance = distance * cos(degree) -5;
        int degree_to_turn = 90+degree;
        float x_distance = abs(distance * cos(degree_to_turn));
        printf("turning 1\n");
        turn_by_angle(abs(degree_to_turn));
        ThisThread::sleep_for(2s);
        printf("straight 1\n");
         car.goStraight(70);
        // go_straight_by_distance(x_distance);
        while(ping_distance*17700.4f-4>42){
            // printf("%dcm",ping_distance);
            ThisThread::sleep_for(100ms);
         }
        //  car.goStraight(100);
        //  ThisThread::sleep_for(1s);
         car.stop();
         ThisThread::sleep_for(1500ms);
         printf("turning 2\n");
         turn_by_angle(-90);
         ThisThread::sleep_for(1500ms);
         printf("straight 2\n");
         car.goStraight(70);
         printf("%dcm",ping_distance*17700.4f-4);
         while(ping_distance*17700.4f-4>30){
            // printf("%dcm",ping_distance);
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
    ThisThread::sleep_for(1500ms);
    turn_by_angle(90);
    xbee.write("Ending AprilTag Calibration Segment\r\n",37);
    stoping_openmv();
    return;
}
void RPC_turnAround (Arguments *in, Reply *out){
    xbee.write("Starting Turn Around Segment\r\n",30);
    turn_by_angle(90);
    ThisThread::sleep_for(1500ms);
    go_straight_by_distance(5);
    ThisThread::sleep_for(1500ms);
    turn_by_angle(-90);
    ThisThread::sleep_for(1500ms);
    go_straight_by_distance(20);
    ThisThread::sleep_for(1500ms);
    turn_by_angle(-90);
    ThisThread::sleep_for(1500ms);
    uart.write("stop",4);    
    xbee.write("Ending Turn Around Segment\r\n",28);
}
void RPC_parkNum (Arguments *in, Reply *out){
    int parking_slot = in->getArg<double>();
    int distance_to_go = 0;
    xbee.write("Starting Parking Segment\n",25);
    switch (parking_slot)
    {
    case 1:
        distance_to_go = 30;
        break;
    case 2:
        distance_to_go = 45;
        break;
    case 3:
        distance_to_go = 60;
        break;
    case 4:
        distance_to_go = 75;
        break;
    default:
        break;
    }
    car.goStraight(70);
    printf("%dcm",ping_distance*17700.4f-4);
    while(ping_distance*17700.4f-4>20){
        // printf("%dcm",ping_distance);
        ThisThread::sleep_for(100ms);
    }
    // encoder_ticker.attach(&encoder_control, 10ms);
    // steps_left = 0;
    // steps_right = 0;
    // last_left = 0;
    // last_right = 0;
    // while(steps_left*6.5*PI/32 <= distance_to_go) {
    //     ThisThread::sleep_for(100ms);
    // }
    stoping_openmv();
    car.stop();
    ThisThread::sleep_for(100ms);
    turn_by_angle(-90);
    ThisThread::sleep_for(100ms);
    go_straight_by_distance(15);
    xbee.write("Ending Parking Segment\n",23);
}

RPCFunction rpcRunDist(&RPC_goStraightByDistance, "goDistance");
RPCFunction rpcTurnByAngle(&RPC_turnAngle, "turnAngle");
RPCFunction rpcRevTurnByAngle(&RPC_turnAngleRev, "turnAngleRev");
RPCFunction rpcAnalyse(&RPC_analyse, "parking");
RPCFunction rpcAprilTag(&RPC_aprilTag, "apriltag_calibration");
RPCFunction rpcTurnAround(&RPC_turnAround, "turnAround");
RPCFunction rpcParkNum(&RPC_parkNum, "parkNum");
/* #endregion my RPC */

/* #region xbee */
void RPC_sendXbee(Arguments *in, Reply *out){
    int msg_mode = in->getArg<double>();
    int msg_num = in->getArg<double>();
    if(msg_mode == 0){  // Line Following
        switch (msg_num)
        {
        case 0:
            xbee.write("Starting Line Following Segment 1\r\n",35);
            break;
        case 1:
            xbee.write("Ending Line Following Segment 1\r\n",33);
            break;
        default:
            break;
        }
    }
    else if(msg_mode==1){
        switch (msg_num)
        {
        case 0:
            xbee.write("Starting TensorFlow Segment\r\n",29);
            break;
        case 1:
            xbee.write("Ending TensorFlow Segment\r\n",27);
            break;
        case 2:
            xbee.write("This is: 1\r\n",12);
            break;
        case 3:
            xbee.write("This is: 2\r\n",12);
            break;
        case 4:
            xbee.write("This is: 3\r\n",12);
            break;
        case 5:
            xbee.write("This is: 4\r\n",12);
            break;
        default:
            break;
        }
    }
    // xbee.
}
RPCFunction rpcPrintXbee(&RPC_sendXbee, "printXbee");

void xbee_rpc() {
   char buf[256], outbuf[256];
   FILE *devin = fdopen(&xbee, "r");
   FILE *devout = fdopen(&xbee, "w");
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
/* #endregion xbee */

/* #region uart */
void stoping_openmv(){
    uart.write("stop",4);
}
void uart_rpc() {
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
/* #endregion uart */

int main() {
    /* #region RPC*/
    xbee_thread.start(xbee_rpc);
    uart_thread.start(uart_rpc);
    /* #endregion RPC*/
    
    /* #region PING */
    xbee.set_baud(9600);
    while(1){

        ping.output();
        ping = 0; wait_us(200);
        ping = 1; wait_us(5);
        ping = 0; wait_us(5);
        ping.input();

        while(ping.read() == 0);
        t.start();
        while(ping.read() == 1);
        ping_distance = t.read();
        printf("Distance = %lf cm\r\n", ping_distance*17700.4f-4);
        t.stop();

        t.reset();
        ThisThread::sleep_for(1s);
    }
    /* #endregion */
    return 0;
}