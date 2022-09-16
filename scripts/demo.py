import sensor, lcd, time
import KPU as kpu

print("1")
lcd.init()
lcd.rotation(2)
sensor.reset()
sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.QVGA)
sensor.set_vflip(1)
sensor.set_hmirror(1)
sensor.set_windowing((224,224))
print("2")
sensor.run(1)
clock = time.clock()
classes = ["person", "bird", "cat", "cow", "dog", "horse", "sheep", "aeroplane", "bicycle", "boat", "bus", "car", "motorbike", "train","bottle", "chair", "diningtable", "pottedplant", "sofa", "tvmonitor"]
print("3")
task = kpu.load(0x300000)
print(task)

print("4")
a = kpu.set_outputs(task, 0, 7, 7, 125)
anchor = (0.57273, 0.677385, 1.87446, 2.06253, 3.33843, 5.47434, 7.88282, 3.52778, 9.77052, 9.16828)
a = kpu.init_yolo2(task, 0.3, 0.3, 5, anchor)

while(True):
    clock.tick()
    img = sensor.snapshot()

    # to greyscale
    #greyscale = img.to_grayscale(copy=True)
    #for x in range(0, img.width()):
        #for y in range(0, img.height()):
            #pixel = greyscale.get_pixel(x,y)
            #img.set_pixel(x,y,(pixel, pixel, pixel))

    code = kpu.run_yolo2(task, img)
    print(clock.fps())
    if code:
        for i in code:
            a = img.draw_rectangle(i.rect())
            a = lcd.display(img)
            for i in code:
                lcd.draw_string(i.x(), i.y(), classes[i.classid()], lcd.RED, lcd.WHITE)
                lcd.draw_string(i.x(), i.y()+12, '%f1.3'%i.value(), lcd.RED, lcd.WHITE)
                print(clock.fps(), classes[i.classid()])
    else:
        a = lcd.display(img)
a = kpu.deinit(task)
