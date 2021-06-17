import pyb, sensor, image, time, math
sensor.reset()
sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.QQVGA)  # we run out of memory if the resolution is much bigger...
sensor.skip_frames(time=2000)
sensor.set_auto_gain(False)  # must turn this off to prevent image washout...
sensor.set_auto_whitebal(False)  # must turn this off to prevent image washout...
clock = time.clock()
f_x = (2.8 / 3.984) * 160  # find_apriltags defaults to this if not set
f_y = (2.8 / 2.952) * 120  # find_apriltags defaults to this if not set
c_x = 160 * 0.5  # find_apriltags defaults to this if not set (the image.w * 0.5)
c_y = 120 * 0.5  # find_apriltags defaults to this if not set (the image.h * 0.5)

def degrees(radians):
    return (180 * radians) / math.pi


uart = pyb.UART(3, 9600, timeout_char=1000)
uart.init(9600, bits=8, parity=None, stop=1, timeout_char=1000)

GRAYSCALE_THRESHOLD = [(0, 54)]
# 设置阈值，如果是黑线，GRAYSCALE_THRESHOLD = [(0, 64)]；

ROIS = [  # [ROI, weight]
    (0, 100, 160, 20, 0.1), 
    (0, 050, 160, 20, 0.3), 
    (0, 000, 160, 20, 0.7)
]

weight_sum = 0  # 权值和初始化
for r in ROIS: weight_sum += r[4]  # r[4] is the roi weight.

sensor.reset()  
sensor.set_pixformat(sensor.GRAYSCALE)  # use grayscale.
sensor.set_framesize(sensor.QQVGA) 
sensor.skip_frames(30)  
sensor.set_auto_gain(False)  
sensor.set_auto_whitebal(False) 
clock = time.clock() 

while (True):
    clock.tick()
    img = sensor.snapshot()
    centroid_sum = 0
    for r in ROIS:
        blobs = img.find_blobs(GRAYSCALE_THRESHOLD, roi=r[0:4], merge=True)
        if blobs: # 查找像素最多的blob的索引。
            largest_blob = 0
            most_pixels = 0
            for i in range(len(blobs)):
                if blobs[i].pixels() > most_pixels: # 目标区域找到的颜色块（线段块）可能不止一个，找到最大的一个，作为本区域内的目标直线
                    most_pixels = blobs[i].pixels()
                    largest_blob = i
            img.draw_rectangle(blobs[largest_blob].rect())
            img.draw_cross(blobs[largest_blob].cx(),
                           blobs[largest_blob].cy())
            centroid_sum += blobs[largest_blob].cx() * r[4]  # r[4] is the roi weight.
            # 计算centroid_sum，centroid_sum等于每个区域的最大颜色块的中心点的x坐标值乘本区域的权值

    center_pos = (centroid_sum / weight_sum)  # Determine center of line.
    deflection_angle = 0
    deflection_angle = -math.atan((center_pos - 80) / 60)
    deflection_angle = math.degrees(deflection_angle)
    print(("ID: %d Tx: %f, Ty %f, Tz %f, Rx %f, Ry %f, Rz %f\r\n" %print_args)
    # uart.write("%1.2f\r\n"%deflection_angle)
    if deflection_angle > 38:
        time.sleep_ms(30)
        command = "/turnAngle/run " + deflection_angle + " \n"
        uart.write(command)
        print("Right!Turn Angle: %1.2f" % deflection_angle)
    elif deflection_angle < 14:
        time.sleep_ms(30)
        command = "/turnAngle/run " + deflection_angle + " \n"
        uart.write(command)
        print("Left! Turn Angle: %1.2f" % deflection_angle)
    else:
        time.sleep_ms(100)
        command = "/goDistance/run 5 \n"
        uart.write(command)
        print("Going Straight! Turn Angle: %1.2f" % deflection_angle)

    # uart.write(("FPS %f\r\n" % cclock.fps()).encode())
