#!/usr/bin/python
import struct


class Sensor:
    def __init__(self, sensor_id, sensor_name, data_type, sensor):
        self.sensor_id = sensor_id
        self.sensor_name = sensor_name
        self.data_type = data_type
        self.sensor = sensor
        self.handle = sensor.getHandle()
        self.value = 0

    def update_data(self, data):
        if self.data_type == 'string':
            self.value = data
        elif self.data_type == 'float':
            data = (struct.unpack('f', data))[0]
            self.value = float(data)

    def __str__(self):
        string = "ID: " + str(self.sensor_id) + \
                 " - " + self.sensor_name + \
                 " - " + "Data type: " + self.data_type + \
                 " - " + "Handle: " + str(self.handle)
        return string
