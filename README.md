# ESL-HW4


## The following explains how the _*Final Project*_ projects work:
In the final project, I've added a few different function since the Homework 4 project.<br>
And the main controlling unit is the __OpenMV__.

#### All the new functions are listed in the following:
* **_Mbed C++ section_**
 * *translate_theo_to_actual_distance* ( _float **distance**_ )
 * *translate_theo_to_actual_right_angle* ( _float **angle**_ )
  > I found that the code provided from the tutorial ( *car.cpp* ) are not very accurate with distance,  <br>
I decided to use *Encoder* to calculate distance and control the car.<br>
However, just the Encoder still isn't the most accurate, <br>
thus I tried to make the distance go through a function (as above two) and use that result to control the car.
 * *go_straight_by_distance* ( _int **distance**_ )
  > This function uses the *translated **distance*** from above to go straight using Encoder.
 * turn_by_angle ( _int **angle**_ )
  > This function turns the car by the *translated **angle*** using Encoder.<br>
  >One wheel will be staying still and the other will turn with a certain distance.<br>
  >The distance will be calculated as the ***arc of a circle*** with center at the ***not-turning***  wheel.<br>
  >Turn right with **positive** angle; Turn left with **negative** angle.
  
 * *reverse_turn_by_angle* ( _int **angle**_ )
  > This is the same as the above function<br>
    but in reversed angle.
* **_Mbed RPC section_**
 * *RPC_goStraightByDistance()*
 * *RPC_turnAngle()*
 * *RPC_turnAngleRev()*
  >  The above three RPC functions will call the respective functions above,<br>
  >  but also displaying the state of the function (*starting*/*ending*) on **Xbee**.
 * *RPC_analyse()*
  > This function is for *Homework 4 Part 1*. <br>
  > For more information, please scroll down.
 * *RPC_aprilTag()*
  > This function is for *Homework 4 Part 2*. <br>
  > For more information, please scroll down. 
 * *RPC_turnAround()*
  > This function is for the car to turn around the object.<br>
  > It is called by **OpenMV** and uses the functions is **Mbed C++ section**.
 * *RPC_parkNum()*
  > This function is for the car to park into its destinated space in the end of the scheme.<br>
  > It is called by **OpenMV** and uses the functions is **Mbed C++ section**.
  
* **_Mbed Xbee & UART (OpenMV) section_**
 * *RPC_sendXbee()*
  > This function is for sending the requested message through **Xbee**.<br>
  > The requested message is sent from **OpenMV**
 * *stoping_openmv()*
  > This function is for **Mbed** to stop the function running in **OpenMV**<br>
  > This function uses *UART*.
* **_OpenMV section_**
 * *following_line()*
  > In this function, **OpenMV** will try to look for two line in front of the cam. Sample Image below:<br>
  > [sample image](/202200096_319848519806036_762838978969566786_n.png)<br>
  > The image is what the CAM sees. Since the CAM is flipped, Up is the ground.<br>
  > The RED line is the right boundary of the thick line; the BLUE line is the left boundary of the thick line.<br>
  > When these two line is detected, OpenMV will take their `line.theta()` value and add them together into `line_theta_normal`.<br>
  > When the car is prefectly centered and straight,  `line_theta_normal` should be equal to `0`.
  > If the car is tilted, `line_theta_normal` will be `>0` or `<0`<br>
  > I used a table to check how much the car should turn with how large of an angle.<br>
  > And **OpenMV** will send the information to **Mbed** and straighten accordingly.<br>
  > The above process will run every 30ms.<br>
  > When starting and ending this function, **OpenMV** will request **Mbed** to send a message to **Xbee**.
 * *rotate_around()*
  > This function just call `RPC_turnAround()` function in **Mbed**<br>
  > When starting and ending this function, **OpenMV** will request **Mbed** to send a message to **Xbee**.
 * *aprilTag_func()*
  > In this function, **OpenMV** will try to look for a AprilTag.<br>
  > With the AprilTag's information, **OpenMV** will process them into an angle and a distance.<br>
  > Then it will send the information to **Mbed** and call `RPC_aprilTag()`<br>
  > When starting and ending this function, **OpenMV** will request **Mbed** to send a message to **Xbee**.
 * *wait_response()*
  > When this function is called, **OpenMV** will wait for a flag from **Mbed** to resume operation of the camera.
 * *tflite_num()*
  > This function is a TFlite function. The camera will try to read a number. 
  > When the number read, it will send it to **Mbed**.<br>
  > Then it will be send to **Xbee**.
  > When starting and ending this function, **OpenMV** will request **Mbed** to send a message to **Xbee**.
  
### The following is the procedure used in *Final Project Demo*:
1. Line following segment
 * `following_line()`
2. Rotate around an object
 * `rotate_around()` & `RPC_turnAround()` & `wait_response()` 
3. AprilTag Detection segment
 * `aprilTag_func()` & `RPC_aprilTag()` & `wait_response()`
4. TensorFlow Lite segment
 * `tflite_num()` 
5. Parking Segment
 * `RPC_parkNum()`


## The following explains how the _Homework 4_ projects work:

In these projects, I tried to use encoder to calculate the distance instead of time.<br>
In the AprilTag/position_calibration project main.cpp file, you can see 4 new functions for navigating the car. (Not including the RPC functions)<br>

1. `go_straight_by_distance( DISTANCE );`<br>
2. `reverse_by_distance( DISTANCE );`<br>
3. `turn_by_angle( ANGLE );`<br>
4. `reserve_turn_by_angle( ANGLE );`<br>

These function let the servo do their thing with a constant speed, and stop the PWM function when the target DISTANCE/ANGLE is achieved.<br>
Using the step detected by encoder, you can calculate that a step turned is roughly equal to  0.638cm. <br>
With this infomation, one can calculate the distance travelled.<br>
The turning functions (3) & (4) turn with a single wheel while the others (1) & (2) turn with two.<br>

## - For the individual _Homework Tasks_ :

### 1_xbee:

With the information provided from the _PC/Python_ through _Zigbee_, d1/d2/direction <br>
the functions can direct the car to parking the space <br>
by reversing to in front of the space and turn 90 degree left or right <br>
depending on the direction and reserving right into the "parking slot".<br>
<br>

### 2_line_following:

The _main.cpp_ function just receive the RPC functions through UART from OpenMV.<br>
The OpenMV tries to detect the line by blobs.<br>
It will detect a line by lining up the three midpoint of the blob.<br>
With the line you can know the angle of the car.<br>
This is partly based on a [tutorial guide](https://book.openmv.cc/example/10-Color-Tracking/black-grayscale-line-following.html) from OpenMV.<br>
<br>

### 3_position_calibration:<br>
The OpenMV scans for an AprilTag, and returns the translation and rotation of the camera.<br>
From the information, the car first turn perpendicular to the AprilTag;<br>
After turning, the car is in position with the AprilTag just like the Zigbee project but in front facing position.<br>
The car go in front _d1_ cm and turn 90 degree and run in front.<br>
However, the car will stop only when the Ping returns a distance close enough.<br>
