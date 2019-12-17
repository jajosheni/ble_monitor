#!/usr/bin/python

from bluepy import btle
import time
from sensor import Sensor


class MyDelegate(btle.DefaultDelegate):
    def __init__(self):
        btle.DefaultDelegate.__init__(self)

    def handleNotification(self, cHandle, data): # override func
        global my_sensors

        for sensor in my_sensors:
            if sensor.handle == cHandle:
                try:
                    sensor.update_data(data)
                    print(sensor.sensor_name + " : " + str(sensor.value))
                except Exception as e:
                    print(e)



def connect_to_device():
    global dev, mac_address
    while True:
        try:
            dev.connect(mac_address)
            break
        except:
            print("."),
            time.sleep(2.0)
            continue


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

    sensor_list = ["Temperature", "Humidity", "Voltage", "Merged"]

    for i in range(4):
        if i == 3:
            dt = 'string'
        else:
            dt = 'float'

        x = Sensor(
            sensor_id=i,
            sensor_name=sensor_list[i],
            data_type=dt,
            sensor=custom_service.getCharacteristics()[i]
        )
        print(x)

        my_sensors.append(x)

    # Setup to turn notifications on

    setup_data = b"\x01\x00"

    for i in range(4):
        notify = my_sensors[i].sensor
        notify_handle = notify.getHandle() + 1
        dev.writeCharacteristic(notify_handle, setup_data, withResponse=True)

    # Main loop --------
    timeout = 10.0

    while True:
        try:
            if dev.waitForNotifications(timeout):
                # handleNotifications() called
                continue

            print("Waiting...")
        except Exception as e:
            print(e)
            print("Reconnecting..."),
            connect_to_device()