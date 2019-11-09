#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <rgb_lcd.h>        // include the grove RGB LCD library
#include<SimpleDHT.h>
/*********************************** Hardware defines ***********************************************
 *  
 ***************************************************************************************************/
#define LEDPIN      15
#define ANALOGUE    A0
#define DHTPIN      16
/******************************* Function prototypes ************************************************
 *  
 ***************************************************************************************************/
void MQTTconnect ( void );
int gettemphumid ( float *temperature, float *humidity);
int i;
/*********************************** Network defines ************************************************
 *  
 ***************************************************************************************************/
#define ssid         "XT1650 5189"    // your hotspot SSID or name
#define password     "30d3323579f1"    // your hotspot password

/*********************************** Adafruit IO defines*********************************************
 *  
 ***************************************************************************************************/
//#define NOPUBLISH      // comment this out once publishing at less than 10 second intervals

#define ADASERVER     "io.adafruit.com"     // do not change this
#define ADAPORT       1883                  // do not change this 
#define ADAUSERNAME   "notty786"               // ADD YOUR username here between the qoutation marks
#define ADAKEY        "82f4bcbb95d649b0823a1413df09fe2c" // ADD YOUR Adafruit key here betwwen marks

/******************************** Global instances / variables***************************************
 *  
 ***************************************************************************************************/
WiFiClient client;    // create a class instance for the MQTT server
rgb_lcd LCD;          // create a class instance of the rgb_lcd class
SimpleDHT22 dht22;

// create an instance of the Adafruit MQTT class. This requires the client, server, portm username and
// the Adafruit key
Adafruit_MQTT_Client MQTT(&client, ADASERVER, ADAPORT, ADAUSERNAME, ADAKEY);

int red = 0, green = 128, blue = 128;             // variables for the RGB backlight colour

/******************************** Feeds *************************************************************
 *  
 ***************************************************************************************************/
//Feeds we publish to

// Setup a feed called LED to subscibe to HIGH/LOW changes
Adafruit_MQTT_Subscribe tempcontrol2 = Adafruit_MQTT_Subscribe(&MQTT, ADAUSERNAME "/feeds/temp-control-2");
Adafruit_MQTT_Publish LED = Adafruit_MQTT_Publish(&MQTT, ADAUSERNAME "/feeds/led");
Adafruit_MQTT_Publish Temperature = Adafruit_MQTT_Publish(&MQTT, ADAUSERNAME "/feeds/temperature");
/********************************** Main*************************************************************
 *  
 ***************************************************************************************************/
void setup() 
{
  pinMode(LEDPIN,OUTPUT);
  Serial.begin(115200);                         // open a serial port at 115,200 baud
  while(!Serial)                                // wait for serial peripheral to initialise
  {
    ;
  }
  delay(10);                                    // additional delay before attempting to use the serial peripheral
  Serial.print("Attempting to connect to ");    // Inform of us connecting
  Serial.print(ssid);                           // print the ssid over serial
  
  WiFi.begin(ssid, password);                   // attemp to connect to the access point SSID with the password
  
  while (WiFi.status() != WL_CONNECTED)         // whilst we are not connected
  {
    delay(500);                                 // wait for 0.5 seconds (500ms)
    Serial.print(".");                          // print a .
  }
  Serial.print("\n");                           // print a new line
  
  Serial.println("Succesfully connected");      // let us now that we have now connected to the access point
  Serial.print("Mac address: ");                // print the MAC address
  Serial.println(WiFi.macAddress());            // note that the arduino println function will print all six mac bytes for us
  Serial.print("IP:  ");                        // print the IP address
  Serial.println(WiFi.localIP());               // In the same way, the println function prints all four IP address bytes
  Serial.print("Subnet masks: ");               // Print the subnet mask
  Serial.println(WiFi.subnetMask());
  Serial.print("Gateway: ");                    // print the gateway IP address
  Serial.println(WiFi.gatewayIP());
  Serial.print("DNS: ");                        // print the DNS IP address
  Serial.println(WiFi.dnsIP());
   
  LCD.begin(16, 2);                             // initialise the LCD
  LCD.setRGB(red, green, blue);                 // set the background colour

  LCD.print(WiFi.localIP());                    // update the display with the local (i.e. board) IP address
  
  MQTT.subscribe(&tempcontrol2);                         // subscribe to the tempcontrol feed
}
/************************************** Loop ********************************************************
 *  
 ***************************************************************************************************/
void loop() 
{  
  // This function call ensures that we have a connection to the Adafruit MQTT server. This will make
  // the first connection and automatically tries to reconnect if disconnected. However, if there are
  // three consecutive failled attempts, the code will deliberately reset the microcontroller via the 
  // watch dog timer via a forever loop.
  MQTTconnect();

  // an example of subscription code
  Adafruit_MQTT_Subscribe *subscription;                    // create a subscriber object instance
  while ( subscription = MQTT.readSubscription(5000) )      // Read a subscription and wait for max of 5 seconds.
  {                                                         // will return 1 on a subscription being read.
    if (subscription == &tempcontrol2)                               // if the subscription we have receieved matches the one we are after
    {
      Serial.print("Recieved from the temperature control subscription:");  // print the subscription out
      Serial.println((char *)tempcontrol2.lastread);                 // we have to cast the array of 8 bit values back to chars to print
      i = atoi((char *)tempcontrol2.lastread); 						// convert the string to an integer

  }
  }
	float temp, humid;									//code to publisg current temp
    gettemphumid(&temp, &humid);
    if ( Temperature.publish(temp) )
    {
      Serial.print("Temp Published");
    if (temp > i)
      {
        digitalWrite (LEDPIN, LOW);
      }
      else
      {
         digitalWrite (LEDPIN, HIGH);
      }
     
    }
    else 
    {
      Serial.print("error 404");
     
    }


	 if ( LED.publish( digitalRead(LEDPIN) ) )			//code to publish current state of LED
    {
      Serial.print("Published");
    }
    else 
    {
      Serial.print("error 404");
    }

    delay(5000);
  
  // keep alive code. If we are only subscribing, we need to ensure that we are connected. If we do not
  // get a response from a ping to the MQQT broker, disconnect such that on the next loop iteration, we reconnect.
  // Note that this is not needed if we publish to the broker every 10 seconds or less.
  // Compile time directives are used to add or remove this code to the sketch. Once you are publishing,
  // simply remove this code from the sketch by commenting out #define NOPUBLISH at the top of the sketch
  #ifdef NOPUBLISH
  if ( !MQTT.ping() ) 
  {
    MQTT.disconnect();
  }
  #endif
}
/******************************* MQTT connect *******************************************************
 *  
 ***************************************************************************************************/
void MQTTconnect ( void ) 
{
  unsigned char tries = 0;

  // Stop if already connected.
  if ( MQTT.connected() ) 
  {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  while ( MQTT.connect() != 0 )                                        // while we are 
  {     
       Serial.println("Will try to connect again in five seconds");   // inform user
       MQTT.disconnect();                                             // disconnect
       delay(5000);                                                   // wait 5 seconds
       tries++;
       if (tries == 3) 
       {
          Serial.println("problem with communication, forcing WDT for reset");
          while (1)
          {
            ;   // forever do nothing
          }
       }
  }
  
  Serial.println("MQTT succesfully connected!");
}

int gettemphumid ( float *temperature, float *humidity)						//code to read current temp and humidity
{
  if ( dht22.read2(DHTPIN, temperature, humidity, NULL) != SimpleDHTErrSuccess )
  {
    Serial.print("Failed to read DHT sensor");
    delay(2000);
    *temperature = -999;
    *humidity = -999;
    return 0;
  }

  Serial.print("Humidity: ");
  Serial.print(*humidity);
  Serial.println(" RH(%)");
  Serial.print("Temperature: ");
  Serial.print(*temperature);
  Serial.println(" *C");
  return 1;
}

