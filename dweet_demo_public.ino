/*
  Simple Dweet sample sketch
  This sketch will take data from attached sensors,
  compile a HTTP GET request and transmit the data
  to Dweet.io.

  From there, you can visualise your data using a
  compatible dashboard app like freeboard.io

 This demo was presented at Mini MakerFair Athens, October 1-2 2016
 
 Circuit:
 * Arduino MKR1000 
 
 * Connections for BMP180
   ===========
   Connect SCL to analog 5    (for Arduino Uno)
   Connect SDA to analog 4    (for Arduino Uno)
   Connect SCL to Digital 12  (for Arduino MKR1000)
   Connect SDA to Digital 11  (for Arduino MKR1000)
   Connect VDD to 3.3V DC
   Connect GROUND to common ground

 * Connections fot the Photo resistor
   ===========
   Connect to analog pin A2 with a 10KOhm resistor 
   in voltage divider configuration
            10KOhm         Photoresistor
   GND ---\/\/\/\/-----*---------O-------- A2
                       |
                       |
                       |
                       5V

 created 6 September 2016
 by Peter Dalmaris
 */

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>
#include <SPI.h>
#include <WiFi101.h>

Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);

char ssid[]   = "your_wifi_id"; //  your network SSID (name)
char pass[]   = "your_wifi_password";    // your network password (use for WPA, or use as key for WEP)

int status    = WL_IDLE_STATUS;

char server[] = "dweet.io";    // name address for Google (using DNS)

// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 80 is default for HTTP):
WiFiClient client;

int ledPin           = 6; //The LED on the MKR1000 will go on when communicating with Dweet
int photoresistorPin = A2;

IPAddress ip_not_communicating(0, 0, 0, 0); //This IP address is self assigned when the MKR1000 can't communicate.

const long interval          = 6000; //Interval in milliseconds
unsigned long previousMillis = 0;   

int client_closed            = 0;

void setup() {

  pinMode(ledPin, OUTPUT);
  
  Serial.begin(9600);
  Serial.println("Pressure Sensor Test"); Serial.println("");
  
  /* Initialise the sensor */
  if(!bmp.begin())
  {
    /* There was a problem detecting the BMP085 ... check your connections */
    Serial.print("Ooops, no BMP085 detected ... Check your wiring or I2C ADDR!");
    while(1);
  }

  connectToWifi();

}

void loop() {


  if (WiFi.localIP() == ip_not_communicating) {  // The MKR1000 wifi can get disconnected randomly. If this happens, reconnect.
    Serial.println("---- CONNECTION LOST, RECONNECTING  ----");
    connectToWifi();
  }

  /*
   *  I have noticed that the MKR1000 with the Wifi101 library can be connected to Wifi while it has lost
   *  it's IP address and can't communicate with the Internet any more. So, status number is returned as 3
   *  while in reality communication is gone.
   *  To deal with this, I check for the local IP address. If it is 0.0.0.0, I know communication is
   *  broken so I have to disconnect and reconnect to wifi.
   * 
   */
    
  digitalWrite(ledPin, LOW);
  
  unsigned long currentMillis = millis();

  if (millis() % 1000 == 0)
  {
     digitalWrite(ledPin, HIGH);
     delay(10);
     digitalWrite(ledPin, LOW);
  }
  
  if (currentMillis - previousMillis >= interval) {

    //If a response from dweet was not received from the previous post,
    //communication might be dead. I have been having a lot of trouble 
    //with this. So, if this happens, reset wifi and try again.
    //I can detect that this has happened if client_closed is still 1.
    Serial.print("\Client closed: ");
    Serial.println(client_closed);
    if (client_closed == 1)
    {
      connectToWifi();
    }

    Serial.print("\nConnection status: ");
    Serial.println(WiFi.status());

    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);
    Serial.print("\nLocal IP address: ");
    Serial.println(WiFi.localIP());
  
  
    digitalWrite(ledPin, HIGH);
    // save the last time you blinked the LED
    previousMillis = currentMillis;

    //Create the GET request
    String get_request   = "";     //Holds the GET request
    get_request         += "/dweet/for/fe-mkr1k-photo?";
    get_request         += "photoresistor=";
    get_request         += analogRead(photoresistorPin);
    get_request         += "&pressure=";
    sensors_event_t event;
    bmp.getEvent(&event);
    get_request        += event.pressure;
    get_request        += "&temperature=";
    float temperature;
    bmp.getTemperature(&temperature);
    get_request        += temperature;

    Serial.println("Starting connection to server...");
    Serial.print("Sending out: ");
    Serial.print(server);
    Serial.println(get_request);
    // if you get a connection, report back via serial:
    if (client.connect(server, 80)) {
      Serial.println("connected to server");
      // Make a HTTP request:
      client.print("GET ");
      client.print(get_request);
      client.println(" HTTP/1.1");
      client.print("Host: ");
      client.println(server);
      client.println("Connection: close");
      client.println();
      Serial.println("Message posted");
      client_closed = 1 ; //Expect a response 
    } else
    {
      Serial.println("Unable to connect to server, message not posted.");
      connectToWifi(); //Try to reconnect to wifi
    }
//    client_closed = 1 ; //Expect a response 
  }

    // if there are incoming bytes available
  // from the server, read them and print them:
  while (client.available() && client_closed == 1) {
//    client_closed = 1;
    char c = client.read();
    Serial.write(c);
  }

//  // if the server's disconnected, stop the client:
  if (!client.connected() && client_closed == 1) {
 Serial.println();
   Serial.println("disconnecting from server.");
   client.flush();
    client.stop();
    client_closed =  0;

  }
}

void connectToWifi() {
// check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue:
    while (true);
  }

  WiFi.disconnect(); //Reset the connection and start again.
  
  Serial.println("About to connect or reconnect to wifi");
  Serial.print("Current wifi status: ");
  Serial.println(WiFi.status());
  WiFi.disconnect();

  // attempt to connect to Wifi network:
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }
  Serial.println("Connected to wifi");
  printWifiStatus();
  client_closed = 0;  // reset the connection status
  
}

 void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
  Serial.print("Connection status: ");
  Serial.println(WiFi.status());


}
