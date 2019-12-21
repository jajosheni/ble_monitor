#!/usr/bin/python
import requests
from socketIO_client import SocketIO, BaseNamespace


class Router:
    def __init__(self, ip_address, port):
        self.ip_address = ip_address
        self.port = port
        # first we need to get cookie headers for connection
        self.socketIO = SocketIO(self.ip_address, self.port, BaseNamespace)

    def send_data(self, data, m_path):
        try:
            self.socketIO.emit('testVerileri', data)
            self.socketIO.wait(seconds=1)
        except Exception as e:
            print(e)

    def generate_data(self, panelID, humidity, temperature, voltage, current, light):
        return {
            'panelId': panelID,
            'current': current,
            'voltage': voltage,
            'light': light,
            'temperature': temperature,
            'moisture': humidity
            }


    def __str__(self):
        string = "IP: " + str(self.ip_address) + \
                " - " + "PORT: " + str(self.port)
        return string