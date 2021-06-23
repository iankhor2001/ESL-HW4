# Find Line Segments Example
#
# This example shows off how to find line segments in the image. For each line object
# found in the image a line object is returned which includes the line's rotation.

# find_line_segments() finds finite length lines (but is slow).
# Use find_line_segments() to find non-infinite lines (and is fast).

enable_lens_corr = False # turn on for straighter lines...

import sensor, image, time, math , pyb, os, tf

sensor.reset()
sensor.set_pixformat(sensor.RGB565) # grayscale is faster
sensor.set_framesize(sensor.QQVGA)
sensor.skip_frames(time = 2000)
sensor.set_auto_gain(False)  # must turn this off to prevent image washout...
sensor.set_auto_whitebal(False)  # must turn this off to prevent image washout...
clock = time.clock()

# All lines also have `x1()`, `y1()`, `x2()`, and `y2()` methods to get their end-points
# and a `line()` method to get all the above as one 4 value tuple for `draw_line()`.

f_x = (2.8 / 3.984) * 160 # find_apriltags defaults to this if not set
f_y = (2.8 / 2.952) * 120 # find_apriltags defaults to this if not set
c_x = 160 * 0.5 # find_apriltags defaults to this if not set (the image.w * 0.5)
c_y = 120 * 0.5 # find_apriltags defaults to this if not set (the image.h * 0.5)

uart = pyb.UART(3,9600,timeout_char=1000)
uart.init(9600,bits=8,parity = None, stop=1, timeout_char=1000)

turn_degree = ( 0,    5,   10,   15,   20,   30,   40,   70)
turn_factor = ( 1, -0.9, -0.8, -0.7, -0.6, -0.5, -0.4, -0.3)
ROI=(0,0,160,40)#Set roi location
#ROI=(40,0,80,40)#Set roi location
#img.draw_rectangle(ROI)

#tags
follow_line=1
stop_follow_line=1
tflite_flag = 0
command_factor=1
############################################################################################
def following_line():
    global stop_follow_line
    global follow_line
    command = "/printXbee/run 0 0\n"  # print "starting following line function\n"
    uart.write(command)

    while(follow_line):
        clock.tick()
        img = sensor.snapshot()
        if enable_lens_corr: img.lens_corr(1.8) # for 2.8mm lens...

        # `merge_distance` controls the merging of nearby lines. At 0 (the default), no
        # merging is done. At 1, any line 1 pixel away from another is merged... and so
        # on as you increase this value. You may wish to merge lines as line segment
        # detection produces a lot of line segment results.

        # `max_theta_diff` controls the maximum amount of rotation difference between
        # any two lines about to be merged. The default setting allows for 15 degrees.

        line_theta_left_blue = 0
        line_theta_right_red = 0
        l_left_length = 21
        l_right_length = 21
        longest_left = 0
        longest_right = 0

        command = "/goStraight/run 70\n"
        print(command)
        uart.write(command)

        for l in img.find_line_segments(ROI,max_theta_diff = 5,merge_distance=0):
        # region data collection ###########################################################
            img.draw_line(l.line(), color = (0, 255, 0))
            if(l.y1()<=3):
                if(longest_left<l.length()):
                    img.draw_line(l.line(), color = (0, 0, 255))
                    line_theta_left_blue = l.theta()
                    l_left_length = l.length()
                    #print(l)
            if(l.y2()<=3):
                if(longest_right<l.length()):
                    img.draw_line(l.line(), color = (255, 0, 0))
                    line_theta_right_red = l.theta()
                    l_right_length = l.length()
                    #print(l)
        line_theta_normal = 180 - (line_theta_left_blue + line_theta_right_red)
        # how much need to turn. negative is need to turn left, positive is turn right
        print ("theta = " + str(line_theta_normal))
        ## endregion data collection ########################################################

        ## region if_line_end ###############################################################

        if(stop_follow_line):
            if(l_right_length<17 and l_left_length<17):
                follow_line = 0
                uart.write("/stop/run \n")

                command = "/printXbee/run 0 1\n"  # print "Stop following line function\n"
                uart.write(command)
                print("END LINE FOLLOWING")
                stop_follow_line = 0
                break
        else:
            line_nu=0
            line_decoded = [0,0,0]
            send="hi\n"
            uart.write(send.encode())
            line_nu = uart.read(20)
            if(line_nu!=None):
                line_decoded = line_nu.decode()
                if line_decoded != "start":
                    print('No response')
                    print(line_decoded)
                    #print(line_nu)
                    if(line_decoded== 'stop'):
                        follow_line=0
                        break
        # endregion if_line_end ############################################################

        # region turn angle ################################################################
        for i in range(8):
            if (abs(line_theta_normal) <= turn_degree[i]):
                if (line_theta_normal < 0):
                    command_factor = -turn_factor[i]
                elif (line_theta_normal >= 0):
                    command_factor = turn_factor[i]
                break
            else:
                if (line_theta_normal < 0):
                    command_factor = -0.2
                elif (line_theta_normal >= 0):
                    command_factor = 0.2
                break


        command_turn = "/turn/run 70 " + str(command_factor) + "\n"
        uart.write(command_turn)
        print(command_turn)
        ## region turn angle ################################################################

############################################################################################
def rotate_around():
    command = "/turnAround/run \n"
    uart.write(command)
############################################################################################
def degrees(radians):
   return (180 * radians) / math.pi

def aprilTag_func():
    aprilTag_tag = 1
    while(aprilTag_tag):
        clock.tick()
        img = sensor.snapshot()
        for tag in img.find_apriltags(fx=f_x, fy=f_y, cx=c_x, cy=c_y): # defaults to TAG36H11
            img.draw_rectangle(tag.rect(), color = (255, 0, 0))
            img.draw_cross(tag.cx(), tag.cy(), color = (0, 255, 0))

            print_args = (tag.x_translation(), tag.y_translation(), tag.z_translation(), \
                    degrees(tag.x_rotation()), degrees(tag.y_rotation()), degrees(tag.z_rotation()))
            # Translation units are unknown. Rotation units are in degrees.
            deg = degrees(tag.y_translation())
            distance = tag.z_translation() * (28/3)

            command = "/apriltag_calibration/run " + str(deg) + " " + str(distance) + " \n"
            uart.write(command)
            print(command)
            aprilTag_tag = 0
            time.sleep(2)
############################################################################################
def wait_response():
    uart = pyb.UART(3,9600,timeout_char=1000)
    uart.init(9600,bits=8,parity = None, stop=1, timeout_char=1000)
    send = 'start'
    while(send=='start'):
        line_nu=0
        line_decoded = [0,0,0]
        uart.write(send.encode())
        line_nu = uart.read()
        if(line_nu!=None):
            #line_decoded = line_nu.decode()
            if line_decoded != "start":
                print('No response')
                print(line_decoded)
                #print(line_nu)
                if(line_decoded== 'stop'):
                    #print('stop')
                    break
        time.sleep_ms(1000)
############################################################################################
def tflite_num():
    command = "/printXbee/run 1 0 \n"
    uart.write(command)
    sensor.reset()                         # Reset and initialize the sensor.
    sensor.set_pixformat(sensor.GRAYSCALE)    # Set pixel format to RGB565 (or GRAYSCALE)
    sensor.set_framesize(sensor.QQVGA)      # Set frame size to QVGA (320x240)
    sensor.skip_frames(time=2000)          # Let the camera adjust.

    net = "trained.tflite"
    labels = [line.rstrip('\n') for line in open("labels.txt")]
    tflite_not_found = 1
    clock = time.clock()
    while(tflite_not_found):
        clock.tick()
        img = sensor.snapshot().negate()
        result = 0
        # default settings just do one detection... change them to search the image...
        for obj in tf.classify(net, img, min_scale=1.0, scale_mul=0.8, x_overlap=0.5, y_overlap=0.5):
            if(max(obj.output())>0.95):
                print("This is : ",labels[obj.output().index(max(obj.output()))])
                tflite_flag = labels[obj.output().index(max(obj.output()))]
                tflite_not_found = 0
    if(tflite_flag==1):
        command = "/printXbee/run 1 2 \n"
    elif(tflite_flag==2):
        command = "/printXbee/run 1 3 \n"
    elif(tflite_flag==3):
        command = "/printXbee/run 1 4 \n"
    elif(tflite_flag==4):
        command = "/printXbee/run 1 2 \n"
    time.sleep_ms(500)
    uart.write(command)
    time.sleep_ms(500)
    uart.write("/turnAngleRev/run 93\n")
    time.sleep_ms(500)
    command = "/printXbee/run 1 1 \n"
    uart.write(command)
    sensor.set_pixformat(sensor.RGB565)
############################################################################################

# main #####################################################################################

following_line()
time.sleep(5)
rotate_around()
time.sleep(15)
wait_response()
aprilTag_func()
wait_response()
time.sleep(20)
tflite_num()
time.sleep(3)
command = "/parkNum/run " + str(tflite_flag) + "\n"
#command = "/parkNum/run " + str(1) + "\n"
uart.write(command)

#follow_line = 1
#following_line()

