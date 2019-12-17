#!/usr/bin/python

import random
import requests
import time

class Router:
    def __init__(self, ip_address, port):
        self.ip_address = ip_address
        self.port = port

    def send_data(self, data):
        try:
            r = requests.post(self.ip_address + ":" + str(self.port), json=data)
            print(r.status_code)
        except Exception as e:
            print(e)

    def generate_data(self, humidity, temperature, voltage):
        return {
            'humidity': humidity,
            'temperature': temperature,
            'voltage': voltage
            }

    def __str__(self):
        string = "IP: " + str(self.ip_address) + \
                " - " + "PORT: " + str(self.port)
        return string
