import sensor, lcd, time
import KPU as kpu
import utime

first_object_detected = False
#lcd.init()
#lcd.rotation(2)
boot_start_time = utime.ticks_ms()
sensor.reset(dual_buff=True)

sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.QVGA)

sensor.set_vflip(1)
sensor.set_hmirror(1)

sensor.set_windowing((224,224))
sensor.run(1)


clock = time.clock()
classes = ["person", "bird", "cat", "cow", "dog", "horse", "sheep", "aeroplane", "bicycle", "boat", "bus", "car", "motorbike", "train","bottle", "chair", "diningtable", "pottedplant", "sofa", "tvmonitor"]


task = kpu.load(0x300000)
a = kpu.set_outputs(task, 0, 7, 7, 125)
anchor = (0.57273, 0.677385, 1.87446, 2.06253, 3.33843, 5.47434, 7.88282, 3.52778, 9.77052, 9.16828)
a = kpu.init_yolo2(task, 0.3, 0.3, 5, anchor)

while(True):
    #clock.tick()
    img = sensor.snapshot()

    code = kpu.run_yolo2(task, img)

    if first_object_detected is False:
        print("detected in ", utime.ticks_ms() - boot_start_time, "ms")
        first_object_detected = True

    if code:
        for i in code:
            a = img.draw_rectangle(i.rect())
            a = lcd.display(img)
            #for i in code:
                #lcd.draw_string(i.x(), i.y(), classes[i.classid()], lcd.RED, lcd.WHITE)
                #lcd.draw_string(i.x(), i.y()+12, '%f1.3'%i.value(), lcd.RED, lcd.WHITE)
                #print(clock.fps(), classes[i.classid()])
    else:
        a = lcd.display(img)
a = kpu.deinit(task)
