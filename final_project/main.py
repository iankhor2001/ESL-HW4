# Find Line Segments Example
#
# This example shows off how to find line segments in the image. For each line object
# found in the image a line object is returned which includes the line's rotation.

# find_line_segments() finds finite length lines (but is slow).
# Use find_line_segments() to find non-infinite lines (and is fast).

enable_lens_corr = False # turn on for straighter lines...

import sensor, image, time, math , pyb

sensor.reset()
sensor.set_pixformat(sensor.RGB565) # grayscale is faster
sensor.set_framesize(sensor.QQVGA)
sensor.skip_frames(time = 2000)
clock = time.clock()

# All lines also have `x1()`, `y1()`, `x2()`, and `y2()` methods to get their end-points
# and a `line()` method to get all the above as one 4 value tuple for `draw_line()`.

uart = pyb.UART(3,9600,timeout_char=1000)
uart.init(9600,bits=8,parity = None, stop=1, timeout_char=1000)

turn_degree = ( 0,    5,   10,   15,   20,   30,   40,   70)
turn_factor = ( 1, -0.9, -0.8, -0.7, -0.6, -0.5, -0.4, -0.3)
ROI=(0,0,160,40)#Set roi location
#ROI=(40,0,80,40)#Set roi location
#img.draw_rectangle(ROI)


follow_line=1
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
    line_left_length = 21
    line_right_length = 21
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
    # endregion data collection ########################################################

    # region if_line_end ###############################################################
    if(l_right_length<17 and l_left_length<17):
        follow_line = 0
        uart.write("/stop/run \n")
        print("END LINE FOLLOWING")
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

    command = "/turn/run 70 " + str(command_factor) + "\n"
    uart.write(command)
    print(command)
    # region turn angle ################################################################

    time.sleep_ms(100)
