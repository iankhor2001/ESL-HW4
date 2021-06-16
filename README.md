# ESL-HW4

In these projects, I tried to use encoder to calculate the distance instead of time.
In the AprilTag/position_calibration project main.cpp file, you can see 4 new functions for navigating the car. (Not including the RPC functions)
(1) go_straight_by_distance( DISTANCE );
(2) reverse_by_distance( DISTANCE );
(3) turn_by_angle( ANGLE );
(4) reserve_turn_by_angle( ANGLE );
These function let the servo do their thing with a constant speed, and stop the PWM function when the target DISTANCE/ANGLE is achieved.
Using the step detected by encoder, you can calculate that a step turned is roughly equal to  0.638cm. 
With this infomation, one can calculate the distance travelled.
The turning functions (3) & (4) turn with a single wheel while the others (1) & (2) turn with two.

The following explains how the projects work:
1_xbee:
With the information provided from the PC/Python through Zigbee, d1/d2/direction 
the functions can direct the car to parking the space 
by reversing to in front of the space and turn 90 degree left or right 
depending on the direction and reserving right into the "parking slot".

2_line_following:
The main.cpp function just receive the RPC functions through UART from OpenMV.
The OpenMV tries to detect the line by blobs.
It will detect a line by lining up the three midpoint of the blob.
With the line you can know the angle of the car.
This is partly based on a tutorial guide from OpenMV.
https://book.openmv.cc/example/10-Color-Tracking/black-grayscale-line-following.html

3_position_calibration:
The OpenMV scans for an AprilTag, and returns the translation and rotation of the camera.
From the information, the car first turn perpendicular to the AprilTag;
After turning, the car is in position with the AprilTag just like the Zigbee project but in front facing position.
The car go in front d1 and turn 90 degree and run in front.
However, the car will stop only when the Ping returns a distance close enough.
