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
BLECharacteristic* aCharacteristic = NULL;

bool deviceConnected = false;
bool oldDeviceConnected = false;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define T_CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a6"
#define H_CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a7"
#define V_CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define A_CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a9"

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

  // Create a temperature BLE Characteristic
  tCharacteristic = pService->createCharacteristic(
                      T_CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  // Create a humidity BLE Characteristic
  hCharacteristic = pService->createCharacteristic(
                      H_CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  // Create a voltage BLE Characteristic
  vCharacteristic = pService->createCharacteristic(
                      V_CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  // Create a merged BLE Characteristic
  aCharacteristic = pService->createCharacteristic(
                      A_CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor
  tCharacteristic->addDescriptor(new BLE2902());
  hCharacteristic->addDescriptor(new BLE2902());
  vCharacteristic->addDescriptor(new BLE2902());
  aCharacteristic->addDescriptor(new BLE2902());

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

char mString[32];
char txString[8];
char hxString[8];
char vxString[8];
float temperature = 22.11;
float humidity = 0.67;
float voltage = 10.20;

void update_data(){

  //use the functions which are supplied by library.
  humidity = dht.readHumidity();
  // Read temperature as Celsius (the default)
  temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    humidity = -10.0;
    temperature = 1000.0;
    return;
  }
  
  voltage += 1.0;
  dtostrf(temperature, 1, 2, txString);
  dtostrf(humidity, 1, 2, hxString);
  dtostrf(voltage, 1, 2, vxString);
  sprintf(mString,"%s %s %s", txString, hxString, vxString);
        
}

void loop() {
    // notify changed value
    if (deviceConnected) {
      update_data();
      tCharacteristic->setValue(temperature);
      tCharacteristic->notify();

      hCharacteristic->setValue(humidity);
      hCharacteristic->notify();

      vCharacteristic->setValue(voltage);
      vCharacteristic->notify();        
      
      aCharacteristic->setValue(mString);
      aCharacteristic->notify();
      
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