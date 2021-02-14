#include <Wire.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>

//-------------THERMAL PRINTER -------------
int led = 13;
int SW = 2;

// Programe flow related operations

int is_switch_press = 0; // For detecting the switch press status
int debounce_delay = 300; //Debounce delay

//--------------------------------------
//------------------I2C OLED DISPLAY-----------

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

//---------------------------------------------

unsigned char buf[2000];
unsigned char opt_sensors;

unsigned int status = buf[0];
float temperatureC = (buf[1] * 256.0 + buf[2]) / 10.0;
float temperatureF = ((temperatureC * 9.0) / 5.0) + 32.0;
float Humidity = (buf[3] * 256.0 + buf[4]) / 10.0;
unsigned int light = (buf[5] * 256.0 + buf[6]);
unsigned int audio = (buf[7] * 256.0 + buf[8]);
float batVolts = ((buf[9] * 256.0 + buf[10]) / 1024.0) * (3.3 / 0.330);
unsigned int co2_ppm = (buf[11] * 256.0 + buf[12]);
unsigned int voc_ppm = (buf[13] * 256.0 + buf[14]);
char pir[15] = "\0";
//-------------BLE details--------------------------------------------------

char ble_data_format[50] = "%.2f,%.2f,%d,%.2f,%d,%d,%s";
char ble_data[sizeof(ble_data_format) + 5];
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

//-------------BLE details--------------------------------------------------

void restart_info();
//-----------------BLE Characteristic Callbacks--------------------------------------------------
class MyCallbacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic)
    {
      std::string value = pCharacteristic->getValue();
      if (value.length() > 0)
      {
        Serial.println("*********");
        Serial.print("New value: ");
        for (int i = 0; i < value.length(); i++)
          Serial.print(value[i]);
        Serial.println();
        Serial.println("*********");
      }
    }
    void onRead(BLECharacteristic *pCharacteristic)
    {
      pCharacteristic->setValue(ble_data);
    }
};
//-----------------BLE Characteristic Callbacks--------------------------------------------------
//-----------------This is BLE code--------------------------------------------------
void BLE_init()
{
  BLEDevice::init("ESP-32test");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ
                                         // | BLECharacteristic::PROPERTY_WRITE
                                       );

  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->setValue("Hello World");
  pService->start();
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();
}

//-----------------This is BLE code--------------------------------------------------

void setup()
{
  Wire.begin(); // join i2c bus (address optional for master)
  Serial.begin(115200); // start serial for output monitor
  BLE_init();
  restart_info();
  //-------------THERMAL PRINTER SETUP--------
   pinMode(led, OUTPUT);
 pinMode(SW, INPUT);
 digitalWrite(SW,HIGH);

 //--------------------------------------------
 //-------------OLED DISPLAY
 
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  
 
  
  delay(2000);
  display.clearDisplay();
  display.setTextColor(WHITE);
  //-------------------------------------

  Serial.print("temperatureC");
  Serial.print("\t");
  Serial.print("Humidity");
  Serial.print("\t");
  Serial.print("light");
  Serial.print("\t");
  Serial.print("co2_ppm");
  Serial.print("\t\t");
  Serial.print("voc_ppm");
  Serial.print("\t\t");
  Serial.print("batVolts");
  Serial.print("\t\t");
  Serial.print("PIR_EVENT\n");




}

void restart_info()
{
  Wire.beginTransmission(0x2A); // transmit to device
  Wire.write(byte(0x80)); // sends instruction to read firmware version
  Wire.endTransmission(); // stop transmitting
  Wire.requestFrom(0x2A, 1); // request byte from slave device
  unsigned char fw_ver = Wire.read(); // receive a byte
  Wire.beginTransmission(0x2A); // transmit to device
  Wire.write(byte(0x81)); // sends instruction to read firmware subversion
  Wire.endTransmission(); // stop transmitting
  Wire.requestFrom(0x2A, 1); // request byte from slave device
  unsigned char fw_sub_ver = Wire.read(); // receive a byte
  Wire.beginTransmission(0x2A); // transmit to device
  Wire.write(byte(0x82)); // sends instruction to read optional sensors byte
  Wire.endTransmission(); // stop transmitting
  Wire.requestFrom(0x2A, 1); // request byte from slave device
  opt_sensors = Wire.read(); // receive a byte
  delay(1000);
}

//Loop starts here
void loop()
{
  // All sensors except the CO2 sensor are scanned in response to this command
  Wire.beginTransmission(0x2A); // transmit to device
  // Device address is specified in datasheet
  Wire.write(byte(0xC0)); // sends instruction to read sensors in next byte
  Wire.write(0xFF);       // 0xFF indicates to read all connected sensors
  Wire.endTransmission(); // stop transmitting
  // Delay to make sure all sensors are scanned by the AmbiMate
  delay(100);
  Wire.beginTransmission(0x2A); // transmit to device
  Wire.write(byte(0x00));       // sends instruction to read sensors in next byte
  Wire.endTransmission();       // stop transmitting
  Wire.requestFrom(0x2A, 15);   // request 6 bytes from slave device
  // Acquire the Raw Data
  unsigned int i = 0;

  while (Wire.available()) // slave may send less than requested
  {
    buf[i] = 0;
    buf[i] = Wire.read(); // receive a byte as character and store in buffer
    i++;
  }

  status = buf[0];
  temperatureC = (buf[1] * 256.0 + buf[2]) / 10.0;
  temperatureF = ((temperatureC * 9.0) / 5.0) + 32.0;
  Humidity = (buf[3] * 256.0 + buf[4]) / 10.0;
  light = (buf[5] * 256.0 + buf[6]);
  audio = (buf[7] * 256.0 + buf[8]);
  batVolts = ((buf[9] * 256.0 + buf[10]) / 1024.0) * (3.3 / 0.330);
  co2_ppm = (buf[11] * 256.0 + buf[12]);
  voc_ppm = (buf[13] * 256.0 + buf[14]);

  Serial.print(temperatureC, 1);
  Serial.print("\t\t");
  Serial.print(Humidity, 1);
  Serial.print("\t\t");
  Serial.print(light);
  Serial.print("\t");
  Serial.print(co2_ppm);
  Serial.print("\t\t");
  Serial.print(voc_ppm);
  Serial.print("\t\t");
  Serial.print(batVolts);
  Serial.print("\t\t");

  if (status & 0x80)
  {
    Serial.print("  PIR_EVENT");
    strcpy(pir, "PIR EVENT");
  }
  else
    strcpy(pir, "\0");

  Serial.print("\n");
  delay(500);


  snprintf(ble_data, sizeof(ble_data), ble_data_format, temperatureC, Humidity, light, co2_ppm, voc_ppm, batVolts, pir);
  //  Serial.println(ble_data);

  //-------------OLED DISPLAY
   display.clearDisplay();
  // display temperature
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("Temperature: ");
  display.setTextSize(1);
  display.setCursor(0,9);
  display.print(temperatureC, 1);
  display.print(" ");
  display.setTextSize(1);
  display.cp437(true);
  display.write(167);
  display.setTextSize(1);
  display.print("C");
  
  // display humidity
  display.setTextSize(1);
  display.setCursor(0, 20);
  display.print("Humidity: ");
  display.setTextSize(1);
  display.setCursor(0, 31);
  display.print(Humidity, 1);
  display.print(" %"); 

  ////////////////
    display.setTextSize(1);
  display.setCursor(85,0);
  display.print("LIGHT: ");
  display.setTextSize(1);
  display.setCursor(90,11);
  display.print(light, 1);
  display.print(" ");
  display.setTextSize(1);
  display.cp437(true);
//  display.write(167);
//  display.setTextSize(2);

  display.setTextSize(1);
  display.setCursor(85,20);
  display.print("CO2: ");
  display.setTextSize(1);
  display.setCursor(81,31);
  display.print(co2_ppm, 1);
  display.print(" ");
  display.setTextSize(1);
  display.cp437(true);

  
   display.setTextSize(1);
  display.setCursor(0,42);
  display.print("VOC: ");
  display.setTextSize(1);
  display.setCursor(0,52);
  display.print(voc_ppm, 1);
  display.print(" ");
  display.setTextSize(1);
  display.cp437(true);
//  display.write(167);
//  display.setTextSize(2);

  display.setTextSize(1);
  display.setCursor(32,42);
  display.print("BATVLTG: ");
  display.setTextSize(1);
  display.setCursor(42,52);
  display.print(batVolts, 1);
  display.print(" ");
  display.setTextSize(1);
  display.cp437(true);

   display.setTextSize(1);
  display.setCursor(85,42);
  display.print("PIR: ");
  display.setTextSize(1);
  display.setCursor(72,52);
  display.print(pir);
  display.print(" ");
  display.setTextSize(1);
  display.cp437(true);
  
  display.display();

  delay(1000);
  //---------------OLED DISPLAY-------
}
