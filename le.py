#!/usr/bin/python

from bluepy import btle
from threading import Thread
import time
import sys
from sensor import Sensor
from my_router import Router

class MyDelegate(btle.DefaultDelegate):
    def __init__(self):
        btle.DefaultDelegate.__init__(self)

    def handleNotification(self, cHandle, data):  # override func
        global my_sensors, temperature, humidity, voltage, current, light

        for sensor in my_sensors:
            if sensor.handle == cHandle:
                try:
                    sensor.update_data(data)
                    if sensor.sensor_id == 0:
                        temperature = sensor.value
                    elif sensor.sensor_id == 1:
                        humidity = sensor.value
                    elif sensor.sensor_id == 2:
                        voltage = sensor.value
                    elif sensor.sensor_id == 3:
                        light = sensor.value

                except Exception as e:
                    print(e)


def connect_to_device():
    global dev, mac_address
    while True:
        try:
            dev.connect(mac_address)
            break
        except Exception as e:
            print(e)
            time.sleep(2.0)
            continue


def notif_thread():
    global stop_threads
    while True:
        if stop_threads:
            print("  Exiting notif.")
            break
        try:
            if dev.waitForNotifications(5.0):
                # handleNotifications() called
                continue

            print("Waiting...")
        except Exception as e:
            print(e)
            print("Reconnecting..."),
            connect_to_device()


def sync_thread():
    global stop_threads, panel_id, temperature, humidity, voltage, current, light
    r = Router("192.168.43.228", 1337)

    while True:
        if stop_threads:
            print("  Exiting sync.")
            break

        data = r.generate_data(
            panelID=panel_id,
            temperature=temperature,
            humidity=humidity,
            voltage=voltage,
            current=current,
            light=light
        )
        print(data)
        r.send_data(data, 'testVerileri')

        time.sleep(1.0)


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

    setup_uuid = btle.UUID(my_services[2].uuid)  # SETUP SERVICE
    setup_service = dev.getServiceByUUID(setup_uuid)
    panel = setup_service.getCharacteristics()[0]
    panel_id = panel.read()

    threshold_value = "0.2"
    s = setup_service.getCharacteristics()[1]
    sHandle = s.getHandle()
    dev.writeCharacteristic(sHandle, threshold_value, withResponse=True)

    data_uuid = btle.UUID(my_services[3].uuid)  # DATA SERVICE
    data_service = dev.getServiceByUUID(data_uuid)

    sensor_list = ["Temperature", "Humidity", "Voltage", "Light"]

    for i in range(len(sensor_list)):
        x = Sensor(
            sensor_id=i,
            sensor_name=sensor_list[i],
            data_type='float',
            sensor=data_service.getCharacteristics()[i]
        )
        print(x)

        my_sensors.append(x)

    # Setup to turn notifications on

    setup_data = b"\x01\x00"

    for i in range(len(sensor_list)):
        notify = my_sensors[i].sensor
        notify_handle = notify.getHandle() + 1
        dev.writeCharacteristic(notify_handle, setup_data, withResponse=True)

    # Main ----

    temperature = 10.0
    humidity = 20.0
    voltage = 10.0
    current = 5.0
    light = 5.0

    stop_threads = False
    notif = Thread(target=notif_thread, args=())
    sync_data = Thread(target=sync_thread, args=())

    notif.start()
    sync_data.start()

    print('\n\tpress e and then Enter to exit...\n')
    while True:
        ex = raw_input()

        if ex == 'e':
            stop_threads = True
            time.sleep(0.1)
            notif.join()
            sync_data.join()
            sys.exit()

    
