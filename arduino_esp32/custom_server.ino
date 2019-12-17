/*
   Create a BLE server that, once we receive a connection, will send periodic notifications.
   The service advertises itself as: 4fafc201-1fb5-459e-8fcc-c5c9c331914b
   And has a characteristic of: beb5483e-36e1-4688-b7f5-ea07361b26a8

   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create a BLE Characteristic on the Service
   4. Create a BLE Descriptor on the characteristic
   5. Start the service.
   6. Start advertising.

   A connect hander associated with the server starts a background task that performs notification
   every couple of seconds.
*/
#include "DHT.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServer* pServer = NULL;

BLECharacteristic* tCharacteristic = NULL;
BLECharacteristic* hCharacteristic = NULL;
BLECharacteristic* vCharacteristic = NULL;
BLECharacteristic* thresholdCharacteristic = NULL;

bool deviceConnected = false;
bool oldDeviceConnected = false;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define VOLTAGE_UUID        "7b4195e8-2117-11ea-a5e8-2e728ce88125"

#define DHTPIN A14 //D13
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);


class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      BLEDevice::startAdvertising();
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};


float threshold = 1.0;

class WriteCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *thresholdCharacteristic) {
      std::string value = thresholdCharacteristic->getValue();
      
      char val[6];
      boolean flag = true;
      if (value.length() > 0 && value.length() < 6) {
        for (int i = 0; i < value.length(); i++){
          val[i] = value[i];

          if(val[i] >= 48 && val[i] <= 57){
            continue;
          }else if(val[i] == 46){
            continue;
          }else{
            Serial.print("illegal value");
            threshold = 1.0;
            dtostrf(threshold, 1, 2, val);
            thresholdCharacteristic->setValue(val);
            flag = false;
            break;
          }
        }

        if(flag){
          threshold = atof(val);
        }
      }
    }
};


float temperature = 0.0;
float humidity = 0.0;
float voltage = 5.0;

float prev_temp = 0.0;
float prev_hum = 0.0;
float prev_volt = 5.0;

void setup() {
  Serial.begin(115200);
  dht.begin();

  // Create the BLE Device
  BLEDevice::init("ESP32");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a temperature BLE Characteristic -- 0x2A6E - Temperature
  tCharacteristic = pService->createCharacteristic(
                      BLEUUID((uint16_t)0x2A6E),
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  // Create a humidity BLE Characteristic -- 0x2A6F - Humidity
  hCharacteristic = pService->createCharacteristic(
                      BLEUUID((uint16_t)0x2A6F),
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  // Create a voltage BLE Characteristic
  vCharacteristic = pService->createCharacteristic(
                      VOLTAGE_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  // Create a threshold Characteristic -- 0x2A7F - Aerobic Threshold
  thresholdCharacteristic = pService->createCharacteristic(
                      BLEUUID((uint16_t)0x2A7F),
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_WRITE
                    );

  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor
  
  BLEDescriptor tempDescriptor(BLEUUID((uint16_t)0x2901));  // 0x2901 -- client characteristic configuration
  tempDescriptor.setValue("Temperature Value");
  tCharacteristic->addDescriptor(&tempDescriptor);
  tCharacteristic->addDescriptor(new BLE2902());

  BLEDescriptor humDescriptor(BLEUUID((uint16_t)0x2901)); 
  humDescriptor.setValue("Humidity Value");
  hCharacteristic->addDescriptor(&humDescriptor);
  hCharacteristic->addDescriptor(new BLE2902());
  
  
  BLEDescriptor voltageDescriptor(BLEUUID((uint16_t)0x2901)); 
  voltageDescriptor.setValue("Voltage Value");
  vCharacteristic->addDescriptor(&voltageDescriptor);
  vCharacteristic->addDescriptor(new BLE2902());

  thresholdCharacteristic->setCallbacks(new WriteCallbacks());
  char txString[8];
  dtostrf(threshold, 1, 2, txString);
  thresholdCharacteristic->setValue(txString);

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {
    // notify changed value
    if (deviceConnected) {
      humidity = dht.readHumidity();
      temperature = dht.readTemperature();
      voltage += 1.0;
    
      if (isnan(humidity) || isnan(temperature)) {
        Serial.println("Failed to read from DHT sensor!");
        humidity = -10.0;
        temperature = 1000.0;
      }

      if(abs(temperature - prev_temp) > threshold){
        tCharacteristic->setValue(temperature);
        prev_temp = temperature;
        tCharacteristic->notify();
      }

      if(abs(humidity - prev_hum) > threshold){
        hCharacteristic->setValue(humidity);
        prev_hum = humidity;
        hCharacteristic->notify();
      }

      if(abs(voltage - prev_volt) > threshold){
        vCharacteristic->setValue(voltage);
        prev_volt = voltage;
        vCharacteristic->notify(); 
      }  
      
      delay(2*1000); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
    }


    
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
}