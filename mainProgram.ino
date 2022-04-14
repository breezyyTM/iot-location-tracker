#include <SoftwareSerial.h>
#include <TinyGPS++.h>  // for GPS module
#include <Wire.h>  // for 3-axis sensor
//#include <SD.h>  // for SD card

//File myFile;
//int CS = 10;  // CS PIN for SD card

#define DEBUG true
#define DEVICE (0x53) // Device address as specified in data sheet 


// ThingSpeak API key (Write)
String apiKey = "<<WRITE_API_KEY_HERE>>"; // ********************** CHANGE THIS **************************


// Create TinyGPS++ object
TinyGPSPlus gps;

static const int GPSRXPin = 4, GPSTXPin = 5;// We make pin 4 as RX & pin 5 as TX for the GPS module
static const uint32_t GPSBaud = 9600;

SoftwareSerial ss(GPSRXPin, GPSTXPin);


// WiFi Module
SoftwareSerial ESP01(2, 3); // RX, TX

// LED pin
int ledPin = 8;

// Buzzer pin
int buzzerPin = 9;

// DIP Switch Pin
int dipSwitchPin1 = 7;
int dipSwitchPin2 = 6;

// 3-axis Sensor
byte _buff[6];

char POWER_CTL = 0x2D;  //Power Control Register
char DATA_FORMAT = 0x31;
char DATAX0 = 0x32; //X-Axis Data 0
char DATAX1 = 0x33; //X-Axis Data 1
char DATAY0 = 0x34; //Y-Axis Data 0
char DATAY1 = 0x35; //Y-Axis Data 1
char DATAZ0 = 0x36; //Z-Axis Data 0
char DATAZ1 = 0x37; //Z-Axis Data 1



 void setup() {
   pinMode(buzzerPin, OUTPUT); // Buzzer
   pinMode(ledPin, OUTPUT); // LED
//   pinMode(CS, OUTPUT); // SD CARD
   pinMode(dipSwitchPin1, INPUT);  // DIP Switch (1)
   pinMode(dipSwitchPin2, INPUT);  // DIP Switch (2)
   // 3-axis sensor
   Wire.begin();        // join i2c bus (address optional for master)
   Serial.begin(57600); 
   Serial.print("3-axis Init...");
  
   //Put the ADXL345 into +/- 4G range by writing the value 0x01 to the DATA_FORMAT register.
   writeTo(DATA_FORMAT, 0x01);
   
   //Put the ADXL345 into Measurement Mode by writing 0x08 to the POWER_CTL register.
   writeTo(POWER_CTL, 0x08);

   // GPS module
    Serial.begin(19200);
    ss.begin(GPSBaud);


   // WiFi
   Serial.begin(19200);
   while (!Serial) {
   }
   Serial.println("Wi-Fi Starting...");
   
   ESP01.begin(9600);

   // SD Card 
//   Serial.begin(19200);
//   if(!SD.begin(CS)){ //Starting SD card
//    Serial.println("Failed to start SD Card...");
//   }

   // LED 
   digitalWrite(ledPin, HIGH);

//   myFile = SD.open("log.txt", FILE_WRITE);
 }


//===============================================
//               MAIN PROGRAM LOOP
//===============================================

 void loop() {
  //============================
  //  START OF ACTUAL PROGRAM
  //============================

// WIFIIIIIIIIIIIIIIIIIIIIIIIIII
   // Reset ESP8266, put it into mode 1 i.e. STA only, make it join hotspot / AP, 
   // establish single connection
   Serial.println();
   sendData("AT+RST\r\n",2000,DEBUG);
   sendData("AT+CWMODE=1\r\n",2000,DEBUG);
   sendData("AT+CWJAP=\"<<SSID_HERE>>\",\"<<PASSWORD_HERE>>\"\r\n",4000,DEBUG);  
    // ****************************************************************** Change these!
   sendData("AT+CIPMUX=0\r\n",2000,DEBUG);
   Serial.println("hi");


////   // Issue AT commands from serial monitor, to test ESP01:
//   if (Serial.available()) {
//     // UNO receives AT commands from hardware serial/PC, and sends to software serial/ESP01.
//     ESP01.write(Serial.read());
//   }
////   
//   if (ESP01.available()) {
////     // UNO receives responses from software serial/ESP01, and sends to hardware serial/PC. 
//     Serial.write(ESP01.read());

//=========================================== MAIN PART

   if (digitalRead(dipSwitchPin1) == 1){
//     if(!myFile){
//       Serial.println("Opening File");
//       myFile = SD.open("log.txt", FILE_WRITE);
//     }
     int itemid = 1;
     Serial.println("Item: " + itemid);
     int x = readAccelX(); // read the x tilt
     int y = readAccelY(); // read the y tilt
     Serial.println("The value of the x tilt is: " + String(x));
     Serial.println("The value fo the x tilt is: " + String(y));
     delay(1000);
     double longVal = displayFakeInfoLong();
     double latVal = displayFakeInfoLat();
     Serial.println("Latitude: "+String(latVal));
     Serial.println("Longitude: "+String(longVal));
     if(longVal>1.31115 || latVal>103.77910 || longVal <1.30969 || latVal <103.77796){
      digitalWrite(buzzerPin, HIGH); 
     } else {
      digitalWrite(buzzerPin, LOW); 
     }
//     time_t t = now();
//     String logging =" ItemID: "+ String(itemid)+" Longitude: "+ String(longVal)+" Latitude: "+ String(latVal)+ " X-Tilt: "+ String(x)+" Y-Tilt: "+ String(y);
//     delay(1000);
//     if(myFile){
//       Serial.println("Writing to SD Card");
//       myFile.println("Longitude: "+ String(longVal)+" Latitude: "+ String(latVal));
//       myFile.close();
//     }
          // Make TCP connection
      String cmd = "AT+CIPSTART=\"TCP\",\"";
      cmd += "184.106.153.149"; // Thingspeak.com's IP address  
      // ****************************************************************** Change this!
      cmd += "\",80\r\n";
      sendData(cmd,2000,DEBUG);
    
      // Prepare GET string
      String getStr = "GET /update?api_key=";
      getStr += apiKey;
      getStr +="&field1=";
      getStr += longVal;
      getStr +="&field2=";
      getStr += latVal;
      getStr += "\r\n";
      
      // Send data length & GET string
      ESP01.print("AT+CIPSEND=");
      ESP01.println (getStr.length());
      Serial.print("AT+CIPSEND=");
      Serial.println (getStr.length());  
      delay(500);
      if( ESP01.find( ">" ) )
      {
        Serial.print(">");
        sendData(getStr,2000,DEBUG);
      }
    //
    //  // Close connection, wait a while before repeating...
      sendData("AT+CIPCLOSE",16000,DEBUG); // thingspeak needs 15 sec delay between updates
   } else if(digitalRead(dipSwitchPin2) == 1){
//       if(!myFile){
//         myFile = SD.open("log.txt", FILE_WRITE);
//       }
       int itemid = 2;
       int x = readAccelX(); // read the x tilt
       int y = readAccelY(); // read the y tilt
       Serial.println("The value of the x tilt is: " + String(x));
       Serial.println("The value fo the x tilt is: " + String(y));
       delay(2000);
//       double longVal = displayFakeInfoLong();
//       double latVal = displayFakeInfoLat();
       double longVal = 1.310;
       double latVal = 103.778;
       Serial.println("Latitude: "+String(latVal));
       Serial.println("Longitude: "+String(longVal));
       if(longVal>1.31115 || latVal>103.77910 || longVal <1.30969 || latVal <103.77796){
        digitalWrite(buzzerPin, HIGH);
       } else {
        digitalWrite(buzzerPin, LOW); 
       }
//       String logging = " ItemID: "+ String(itemid)+" Longitude: "+ String(longVal)+" Latitude: "+ String(latVal)+ " X-Tilt: "+ String(x)+" Y-Tilt: "+ String(y);
//       delay(1000);
//       if(myFile){
//         Serial.println("Writing to SD Card");
//         myFile.println(logging);
//         myFile.close();
//       }
          // Make TCP connection
      String cmd = "AT+CIPSTART=\"TCP\",\"";
      cmd += "184.106.153.149"; // Thingspeak.com's IP address  
      // ****************************************************************** Change this!
      cmd += "\",80\r\n";
      sendData(cmd,2000,DEBUG);
    
      // Prepare GET string
      String getStr = "GET /update?api_key=";
      getStr += apiKey;
      getStr +="&field1=";
      getStr += longVal;
      getStr +="&field2=";
      getStr += latVal;
      getStr += "\r\n";
      
      // Send data length & GET string
      ESP01.print("AT+CIPSEND=");
      ESP01.println (getStr.length());
      Serial.print("AT+CIPSEND=");
      Serial.println (getStr.length());  
      delay(500);
      if( ESP01.find( ">" ) )
      {
        Serial.print(">");
        sendData(getStr,2000,DEBUG);
      }
    //
    //  // Close connection, wait a while before repeating...
      sendData("AT+CIPCLOSE",16000,DEBUG); // thingspeak needs 15 sec delay between updates
   } else {
      digitalWrite(buzzerPin, LOW); 
   }



  //============================
  //  END OF ACTUAL PROGRAM
  //============================
  
   // Reset ESP8266, put it into mode 1 i.e. STA only, make it join hotspot / AP, 
   // establish single connection
//   Serial.println();
//   sendData("AT+RST\r\n",2000,DEBUG);
//   sendData("AT+CWMODE=1\r\n",2000,DEBUG);
//   sendData("AT+CWJAP=\"<<SSID_HERE>>\",\"<<PASSWORD_HERE>>\"\r\n",4000,DEBUG);  
//   // **********************   Change these!   ************************
//   sendData("AT+CIPMUX=0\r\n",2000,DEBUG);
//   Serial.println("hi");

  


//   // Issue AT commands from serial monitor, to test ESP01:
//   if (Serial.available()) {
//     // UNO receives AT commands from hardware serial/PC, and sends to software serial/ESP01.
//     ESP01.write(Serial.read());
//   }
//   
//   if (ESP01.available()) {
//     // UNO receives responses from software serial/ESP01, and sends to hardware serial/PC. 
//     Serial.write(ESP01.read());







  
  




  // Make TCP connection
//  String cmd = "AT+CIPSTART=\"TCP\",\"";
//  cmd += "184.106.153.149"; // Thingspeak.com's IP address  
//  // ****************************************************************** Change this!
//  cmd += "\",80\r\n";
//  sendData(cmd,2000,DEBUG);

  // Prepare GET string
//  String getStr = "GET /update?api_key=";
//  getStr += apiKey;
//  getStr +="&field1=";
//  getStr += longVal;
//  getStr +="&field2=";
//  getStr += latVal;
//  getStr += "\r\n";
//  
//  // Send data length & GET string
//  ESP01.print("AT+CIPSEND=");
//  ESP01.println (getStr.length());
//  Serial.print("AT+CIPSEND=");
//  Serial.println (getStr.length());  
//  delay(500);
//  if( ESP01.find( ">" ) )
//  {
//    Serial.print(">");
//    sendData(getStr,2000,DEBUG);
//  }
//
//  // Close connection, wait a while before repeating...
//  sendData("AT+CIPCLOSE",16000,DEBUG); // thingspeak needs 15 sec delay between updates


  
}


//===========================================
//         END OF MAIN PROGRAM LOOP
//===========================================

String sendData(String command, const int timeout, boolean debug)
{
    String response = "";
    ESP01.print(command);
    long int time = millis();

    while( (time+timeout) > millis())
    {
      while(ESP01.available())
      {
        // "Construct" response from ESP01 as follows 
         // - this is to be displayed on Serial Monitor. 
        char c = ESP01.read(); // read the next character.
        response+=c;
      }  
    }

    if(debug)
    {
      Serial.print(response);
    }
    
    return (response);
}




  // For 3-axis sensor
  int readAccelX() {
    uint8_t howManyBytesToRead = 6;
    readFrom( DATAX0, howManyBytesToRead, _buff); //read the acceleration data from the ADXL345
  
    // each axis reading comes in 10 bit resolution, ie 2 bytes.  Least Significat Byte first!!
    // thus we are converting both bytes in to one int
    int x = (((int)_buff[1]) << 8) | _buff[0];   
    int y = (((int)_buff[3]) << 8) | _buff[2];
    int z = (((int)_buff[5]) << 8) | _buff[4];

    return x;
  }


  int readAccelY() {
    uint8_t howManyBytesToRead = 6;
    readFrom( DATAX0, howManyBytesToRead, _buff); //read the acceleration data from the ADXL345
  
    // each axis reading comes in 10 bit resolution, ie 2 bytes.  Least Significat Byte first!!
    // thus we are converting both bytes in to one int
    int x = (((int)_buff[1]) << 8) | _buff[0];   
    int y = (((int)_buff[3]) << 8) | _buff[2];
    int z = (((int)_buff[5]) << 8) | _buff[4];
    Serial.print("x: ");
    Serial.print( x );
    Serial.print(" y: ");
    Serial.print( y );
    Serial.print(" z: ");
    Serial.println( z );

    return y;
  }
  
  
  void writeTo(byte address, byte val) {
    Wire.beginTransmission(DEVICE); // start transmission to device 
    Wire.write(address);             // send register address
    Wire.write(val);                 // send value to write
    Wire.endTransmission();         // end transmission
  }
  
  
  // Reads num bytes starting from address register on device in to _buff array
  void readFrom(byte address, int num, byte _buff[]) {
    Wire.beginTransmission(DEVICE); // start transmission to device 
    Wire.write(address);             // sends address to read from
    Wire.endTransmission();         // end transmission
  
    Wire.beginTransmission(DEVICE); // start transmission to device
    Wire.requestFrom(DEVICE, num);    // request 6 bytes from device
  
    int i = 0;
    while(Wire.available())         // device may send less than requested (abnormal)
    { 
      _buff[i] = Wire.read();    // receive a byte
      i++;
    }
    Wire.endTransmission();         // end transmission
  }




  // For GPS module
  void displayInfo()
  {
    Serial.print(F("Location: ")); 
    if (gps.location.isValid())
    {
      Serial.print(gps.location.lat(), 6);
      Serial.print(F(","));
      Serial.print(gps.location.lng(), 6);
    }
    else
    {
      Serial.print(F("NO VALUES YET"));
    }
  
    Serial.print(F("  Date "));
    if (gps.date.isValid())
    {
      Serial.print(gps.date.month());
      Serial.print(F("/"));
      Serial.print(gps.date.day());
      Serial.print(F("/"));
      Serial.print(gps.date.year());
    }
    else
    {
      Serial.print(F("INVALID"));
    }
  
    
    Serial.println();
  }


  // Come up with fake GPS values, in case GPS does not work properly
    double displayFakeInfoLong()
  {
    Serial.print(F("Location: ")); 
//    if (gps.location.isValid())
//    {
//      Serial.print(gps.location.lat(), 6);
//      Serial.print(F(","));
//      Serial.print(gps.location.lng(), 6);
//    }
//    else
//    {

        double longVal = random(1, 100) / 100.0;
        double latVal = random(10000, 10100) / 100.0;
        Serial.print(longVal, 6); // Longitude
        Serial.print(F(","));
        Serial.print(latVal, 6); // Latitude
      
//    }
  
    Serial.print(F("  Date "));
    if (gps.date.isValid())
    {
      Serial.print(gps.date.month());
      Serial.print(F("/"));
      Serial.print(gps.date.day());
      Serial.print(F("/"));
      Serial.print(gps.date.year());
    }
    else
    {
      Serial.print(F("INVALID"));
    }
  
    
    Serial.println();

    return longVal;
  }



    double displayFakeInfoLat()
  {
    Serial.print(F("Location: ")); 
//    if (gps.location.isValid())
//    {
//      Serial.print(gps.location.lat(), 6);
//      Serial.print(F(","));
//      Serial.print(gps.location.lng(), 6);
//    }
//    else
//    {

      double longVal = random(1, 100) / 100.0;
      double latVal = random(10000, 10100) / 100.0;
      Serial.print(longVal, 6); // Longitude
      Serial.print(F(","));
      Serial.print(latVal, 6); // Latitude
      
//    }
  
    Serial.print(F("  Date "));
    if (gps.date.isValid())
    {
      Serial.print(gps.date.month());
      Serial.print(F("/"));
      Serial.print(gps.date.day());
      Serial.print(F("/"));
      Serial.print(gps.date.year());
    }
    else
    {
      Serial.print(F("INVALID"));
    }
  
    
    Serial.println();
    return latVal;
  }
