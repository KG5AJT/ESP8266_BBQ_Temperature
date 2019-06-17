#include <ESP8266WiFi.h>							//include appropriate libraries for functions used
#include <PubSubClient.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_ADS1015.h>

Adafruit_ADS1015 ads(0x48);							//declaration for analog digital converter


#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

#define OLED_RESET 16
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);	// declaration for the I2C OLED

const char* ssid = "YOURSSID";							//variables for WiFi & MQTT connectivity
const char* password = "YOURPASSWORD";

const char* mqttServer = "YOURMQTTSERVER";
const int   mqttPort = YOURPORT#;
const char* mqttUser = "YOURMQTTUSER";
const char* mqttPassword = "YOURMQTTPASSWORD";

WiFiClient espClient;								//declarations for WiFi and MQTT initialization
PubSubClient client(espClient);

char mqttmsg[8];
float tempK, tempC, tempF;
											//function prototypes
void callback(char* topic, byte* payload, unsigned int length);				//recieve MQTT messages
void initiateDisplay();									//connect to OLED
void initiateWiFi();									//connect to WiFi
void initiateMQTT();									//connect to MQTT
void getTemps(int probe, int resistance, float &tempK, float &tempC, float &tempF);	//read temperature probes

///////////////////////////////////////////////////////////////////////////////////////////////
void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  initiateDisplay();

  initiateWiFi();
  WiFi.mode(WIFI_STA);
  initiateMQTT();

  ads.begin();
  ads.setGain(GAIN_ONE);

}
////////////////////////////////////////////////////////////////////////////////////////////////
void loop() {

  getTemps(0, 100300, tempK, tempC, tempF);		//read temperature of probe 0
  dtostrf(tempF, 6, 2, mqttmsg);
  client.publish("0", mqttmsg);				//send reading to MQTT server
  Serial.println(tempF);
  
  display.clearDisplay();				//display reading on OLED
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("  Probe 0:");
  display.print("   ");
  display.print(tempF);
  display.display();
  delay(5000);
  
  
  getTemps(1, 100500, tempK, tempC, tempF);		//read temperature of probe 1
  dtostrf(tempF, 6, 2, mqttmsg);
  client.publish("1", mqttmsg);				//send reading to MQTT server
  Serial.println(tempF);

  display.clearDisplay();				//display reading on OLED	
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("  Probe 1:");
  display.print("   ");
  display.print(tempF);
  display.display();
  delay(5000);





  client.loop();					//send MQTT buffers
  
   client.publish("sync", "online");			//publish verification that device is still online
  
   if (!client.connected()) {				//if MQTT disconnects, check WiFi and MQTT connection
       initiateWiFi();
       initiateMQTT();
   }
}
/////////////////////////////////////////////////////////////////////////////////////////////////
void callback(char* topic, byte * payload, unsigned int length) {
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  Serial.print("Message: ");
  for (int i = 0; i < length; ++i) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  Serial.println("-------------------------");
}
/************************************************************************************************/
void initiateDisplay() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }

  //Clear the buffer
  display.display();
  delay(2000);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
}
/************************************************************************************************/
void initiateWiFi() {
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  Serial.println("WiFi Connected");
}
/************************************************************************************************/
void initiateMQTT() {
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  Serial.println("Connecting to MQTT...");
  while (!client.connected()) {

    if (client.connect("ESP8266Client", mqttUser, mqttPassword)) {
      Serial.println("MQTT Connected");
      Serial.println("Device Active");
    }
    else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }
}
/************************************************************************************************/
void getTemps(int probe, int resistance, float &tempK, float &tempC, float &tempF) {

  float avgArray[15];
  float avg = 0.0;
  float voltage;
  float reading;

  for (int i = 0; i <= 14; i++) {					//take 15 readings from temp probe and average them
    reading = ads.readADC_SingleEnded(probe);
    voltage = (reading * 2) / 1000;
    avgArray[i] = (((3.3 / voltage) - 1) * resistance);
    delay(250);
  }
  for (int i = 0; i <= 14; i++) {
    avg = avg + avgArray[i];
  }
  avg = avg / 15;
									//use Steinhart and Hart equation to calculate temperature
  tempK = 1.0 / ((7.432836 * pow(10, -4)) + ((2.140684 * pow(10, -4)) * log(avg)) + ((9.782938 * pow(10, -8)) * (pow(log(avg), 3))));
  tempC = tempK - 273.15;
  tempF = (tempC * 1.8) + 32;						//convert calculated temperature in Kelvin to Celcius and Faranheit
}
