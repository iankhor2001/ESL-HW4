import sensor, image, time, math , pyb

sensor.reset()
sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.QQVGA) # we run out of memory if the resolution is much bigger...
sensor.skip_frames(time = 2000)
sensor.set_auto_gain(False)  # must turn this off to prevent image washout...
sensor.set_auto_whitebal(False)  # must turn this off to prevent image washout...
clock = time.clock()

f_x = (2.8 / 3.984) * 160 # find_apriltags defaults to this if not set
f_y = (2.8 / 2.952) * 120 # find_apriltags defaults to this if not set
c_x = 160 * 0.5 # find_apriltags defaults to this if not set (the image.w * 0.5)
c_y = 120 * 0.5 # find_apriltags defaults to this if not set (the image.h * 0.5)

def degrees(radians):
   return (180 * radians) / math.pi

while(True):
    clock.tick()
    img = sensor.snapshot()


    uart = pyb.UART(3,9600,timeout_char=1000)
    uart.init(9600,bits=8,parity = None, stop=1, timeout_char=1000)

    for tag in img.find_apriltags(fx=f_x, fy=f_y, cx=c_x, cy=c_y): # defaults to TAG36H11
        img.draw_rectangle(tag.rect(), color = (255, 0, 0))
        img.draw_cross(tag.cx(), tag.cy(), color = (0, 255, 0))

        print_args = (tag.x_translation(), tag.y_translation(), tag.z_translation(), \
                degrees(tag.x_rotation()), degrees(tag.y_rotation()), degrees(tag.z_rotation()))
        # Translation units are unknown. Rotation units are in degrees.
        deg = degrees(tag.y_translation())
        distance = tag.z_translation() * (28/3)

        command = "/apriltag_calibration/run " + deg + " " + distance + " \n"
        uart.write(command)
        time.sleep(2)


        print("Tx: %f, Ty %f, Tz %f, Rx %f, Ry %f, Rz %f" % print_args)
    #print(clock.fps())
