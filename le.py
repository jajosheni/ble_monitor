#!/usr/bin/python

from bluepy import btle
import time
from sensor import Sensor
from my_router import Router


class MyDelegate(btle.DefaultDelegate):
    def __init__(self):
        btle.DefaultDelegate.__init__(self)

    def handleNotification(self, cHandle, data): # override func
        global my_sensors, temperature, humidity, voltage

        for sensor in my_sensors:
            if sensor.handle == cHandle:
                try:
                    sensor.update_data(data)
                    if sensor.sensor_id == 0:
                        temperature = sensor.value
                    if sensor.sensor_id == 1:
                        humidity = sensor.value
                    if sensor.sensor_id == 2:
                        voltage = sensor.value
                except Exception as e:
                    print(e)



def connect_to_device():
    global dev, mac_address
    while True:
        try:
            dev.connect(mac_address)
            break
        except Exception as e:
            print(e),
            time.sleep(2.0)
            continue



def notif_thread():
    while True:
        try:
            if dev.waitForNotifications(5.0):
                # handleNotifications() called
                continue

            print("Waiting...")
        except Exception as e:
            print(e)
            print("Reconnecting..."),
            connect_to_device()


my_sensors = []

if __name__ == "__main__":

    # Initialisation  -------

    mac_address = "b4:e6:2d:86:5b:a7"
    dev = btle.Peripheral()

    print("Connecting...")
    connect_to_device()
    dev.setDelegate(MyDelegate())

    print("Services: ")
    my_services = None
    while True:
        try:
            my_services = dev.getServices()
            for svc in my_services:
                print(svc)
            break
        except:
            connect_to_device()

    custom_uuid = btle.UUID(my_services[2].uuid)  # 0 & 1 are Generic Attribute and Service
    custom_service = dev.getServiceByUUID(custom_uuid)

    sensor_list = ["Temperature", "Humidity", "Voltage"]

    for i in range(len(sensor_list)):
        x = Sensor(
            sensor_id=i,
            sensor_name=sensor_list[i],
            data_type='float',
            sensor=custom_service.getCharacteristics()[i]
        )
        print(x)

        my_sensors.append(x)

    # Setup to turn notifications on

    setup_data = b"\x01\x00"

    for i in range(len(sensor_list)):
        notify = my_sensors[i].sensor
        notify_handle = notify.getHandle() + 1
        dev.writeCharacteristic(notify_handle, setup_data, withResponse=True)

    # Main loop --------

    notif = Thread(target=notif_thread, args=())
    notif.start()
    notif.join()

    r = Router("192.168.43.228", 3000)
    temperature = 10.0
    humidity = 20.0
    voltage = 10.0

    while True:
        data = r.generate_data(temperature, humidity, voltage)
        print(data)
        r.send_data(data)
        
        time.sleep(1.0)
