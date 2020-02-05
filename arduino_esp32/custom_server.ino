/*
   Create a BLE server that, once we receive a connection, will send notifications based on data change.
   The setup service advertises itself as: "8be32b1e-2432-11ea-978f-2e728ce88125"
      -Panel ID
      -Threshold
   whereas the data service is:            "8be32db2-2432-11ea-978f-2e728ce88125"
      -Temperature
      -Humidity
      -Current
      -Voltage

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
char * panelID = "56eb3d5d7c727d861b3278ec"; //umuttepe paneli
BLECharacteristic* panelCharacteristic = NULL; //panelID

BLECharacteristic* tCharacteristic = NULL; //temperature
BLECharacteristic* hCharacteristic = NULL; //humidity(moisture)
BLECharacteristic* vCharacteristic = NULL; //voltage
BLECharacteristic* cCharacteristic = NULL; //current

BLECharacteristic* thresholdCharacteristic = NULL;

bool deviceConnected = false;
bool oldDeviceConnected = false;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SETUP_SERVICE_UUID  "8be32b1e-2432-11ea-978f-2e728ce88125"
#define DATA_SERVICE_UUID   "8be32db2-2432-11ea-978f-2e728ce88125"

#define PANEL_UUID          "712b365e-2432-11ea-978f-2e728ce88125"
#define THRESHOLD_UUID      "712b3a8c-2432-11ea-978f-2e728ce88125"

#define TEMPERATURE_UUID    "2A1C" //Temp measurement
#define HUMIDITY_UUID       "4cf26ed8-2432-11ea-978f-2e728ce88125"
#define VOLTAGE_UUID        "4cf27162-2432-11ea-978f-2e728ce88125"
#define CURRENT_UUID        "4cf273c4-2432-11ea-978f-2e728ce88125"

// TEMPERATURE - HUMIDITY SENSOR SETUP
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


float threshold = 0.1;

float temperature = 0.0;
float humidity = 0.0;
float voltage = 5.0;
float current = 1.0;

float prev_temp = 0.0;
float prev_hum = 0.0;
float prev_volt = 5.0;
float prev_curr = 1.0;

class WriteCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *thresholdCharacteristic) {
      std::string value = thresholdCharacteristic->getValue(); // get value from client and write it to threshold
      
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
            threshold = 0.1;
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

void setup() {
  Serial.begin(115200);
  dht.begin();

  // Create the BLE Device
  BLEDevice::init("ESP32");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *sService = pServer->createService(SETUP_SERVICE_UUID);
  BLEService *dService = pServer->createService(DATA_SERVICE_UUID);

  // ----------------------------------------------------------------------------------
  // Create a panelID Characteristic & Add the panelID value
  panelCharacteristic = sService->createCharacteristic(
                      PANEL_UUID,
                      BLECharacteristic::PROPERTY_READ
                    );
  // Create a BLE Descriptor
  // https://www.bluetooth.com/specifications/gatt/characteristics/
  BLEDescriptor panelDescriptor(BLEUUID((uint16_t)0x2902));  // SYSTEM ID
  panelDescriptor.setValue("Panel ID");
  panelCharacteristic->addDescriptor(&panelDescriptor);
  panelCharacteristic->setValue(panelID);

  // ----------------------------------------------------------------------------------
  // Create a threshold Characteristic
  thresholdCharacteristic = sService->createCharacteristic(
                      THRESHOLD_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE
                    );
                    
  thresholdCharacteristic->setCallbacks(new WriteCallbacks());
  char txString[8];
  dtostrf(threshold, 1, 2, txString);
  thresholdCharacteristic->setValue(txString);

  // ----------------------------------------------------------------------------------

  // Create a humidity BLE Characteristic -- BLEUUID((uint16_t)0x2A6F) - Humidity
  hCharacteristic = dService->createCharacteristic(
                      HUMIDITY_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  hCharacteristic->addDescriptor(new BLE2902());
  
  // ----------------------------------------------------------------------------------
  // Create a voltage BLE Characteristic
  vCharacteristic = dService->createCharacteristic(
                      VOLTAGE_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  vCharacteristic->addDescriptor(new BLE2902());

   ----------------------------------------------------------------------------------
//   Create a current BLE Characteristic -- custom service supports only 4 characteristics with active notify
  cCharacteristic = dService->createCharacteristic(
                      CURRENT_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  cCharacteristic->addDescriptor(new BLE2902());
  
  // ----------------------------------------------------------------------------------


  // Create a temperature BLE Characteristic -- BLEUUID((uint16_t)0x2A1C) - Temperature measurement
  tCharacteristic = dService->createCharacteristic(
                      TEMPERATURE_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  BLEDescriptor tempDescriptor(BLEUUID((uint16_t)0x2A1C));
  tempDescriptor.setValue("Temperature Value");
  tCharacteristic->addDescriptor(&tempDescriptor);
  tCharacteristic->addDescriptor(new BLE2902());
  
  // ----------------------------------------------------------------------------------
  // Start the services
  sService->start();
  dService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SETUP_SERVICE_UUID);
  pAdvertising->addServiceUUID(DATA_SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x00);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {
    // notify changed value
    if (deviceConnected) {
      humidity = dht.readHumidity();
      temperature = dht.readTemperature();
      voltage += 0.5;
      current += 0.5;
      Serial.print("----------------\nPanel ID: ");
      Serial.println(panelID);
      Serial.print("humidity: ");
      Serial.println(humidity);
      Serial.print("temperature: ");
      Serial.println(temperature);
      Serial.print("voltage: ");
      Serial.println(voltage);
      Serial.print("current: ");
      Serial.println(current);
      Serial.print("threshold: ");
      Serial.println(threshold);
    
      if (isnan(humidity) || isnan(temperature)) {
        Serial.println("Failed to read from DHT sensor!");
        humidity = -1.0;
        temperature = 100.0;
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

      if(abs(current - prev_curr) > threshold){
        cCharacteristic->setValue(current);
        prev_curr = current;
        cCharacteristic->notify(); 
      }  
      
      delay(1*1000); // bluetooth stack will go into congestion if too many packets are sent
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
