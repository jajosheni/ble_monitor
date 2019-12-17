#!/usr/bin/python

from bluepy import btle
import time
import binascii
import struct
        
class Sensor:

    def __init__(self, sensor_id, sensor_name, data_type,my_service):
        self.sensor_id = sensor_id
        self.sensor_name = sensor_name
        self.data_type = data_type
        self.sensor = my_service.getCharacteristics()[sensor_id]
        self.value = 0
        print(self.sensor_id, self.sensor_name, self.data_type)
        
    def update_data(self):
        data = self.sensor.read() #get data from sensor
        
        if self.data_type == 'string':
            self.value=data
        elif self.data_type == 'float':
            data = (struct.unpack('f', data))[0] 
            self.value=data


if __name__ == "__main__":
        
    print("Connecting...")

    dev = btle.Peripheral("b4:e6:2d:86:5b:a7")

    print("Services: ")
    for svc in dev.services:
        print(svc)

    ESP32 = btle.UUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b")
    my_service = dev.getServiceByUUID(ESP32)

    my_sensors = []
    sensor_names = ["Temperature", "Humidity", "Voltage", "Merged"]
    
    print("Sensors: \n ID\tName\tData Type")
    for i in range(4):
        if i == 3:
            dt = 'string'
        else:
            dt = 'float'
            
        x = Sensor(
        sensor_id = i,
        sensor_name = sensor_names[i],
        data_type = dt
        )
            
        my_sensors.append(x)

    while(True):
        print("-"*60)
        for m_sens in my_sensors:
            m_sens.update_data()
            print(m_sens.sensor_name + " : " + str(m_sens.value))
        time.sleep(1.0)

