#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <ArduinoJson.h>
#include <BH1750.h>
#include <Wire.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include "DHT.h"
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
FirebaseData firebasedata;
LiquidCrystal_I2C lcd(0x27,16,2);
#define DHTPIN 14     // what digital pin we're connected to
#define DHTTYPE DHT11 // DHT 11
byte degree[8] = {
  0B01110,
  0B01010,
  0B01110,
  0B00000,
  0B00000,
  0B00000,
  0B00000,
  0B00000
};
#define FIREBASE_HOST "smarthome-d9efb-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "2asbijf10Az16fFyBMEc9fiUJtWqT46B9OIE0Iw5"

#define WIFI_SSID "Quan"   //tên wifi
#define WIFI_PASSWORD "quanprovip" //password wifi
DHT dht(DHTPIN, DHTTYPE);
String Led_Button = "";
BH1750 lightMeter(0x23);
int led = D3;
void setup(){
   Serial.begin(9600);
   delay(1000);
   pinMode(LED_BUILTIN, OUTPUT);
   pinMode(led, OUTPUT);
   WiFi.begin(WIFI_SSID, WIFI_PASSWORD); //kết nối với wifi
   Serial.print("Connecting to ");
   Serial.print(WIFI_SSID);
   while (WiFi.status() != WL_CONNECTED) {
     Serial.print(".");
     delay(500);
   }
   Serial.println();
   Serial.print("Connected to ");
   Serial.println(WIFI_SSID);
   Serial.print("IP Address is : ");
   Serial.println(WiFi.localIP()); //In ra địa chỉ IP
   Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH); // Kết nối tới firebase
   dht.begin();
   Wire.begin();
   if (lightMeter.begin()) {
    Serial.println(F("BH1750 initialised"));
   }
   else {
    Serial.println(F("Error initialising BH1750"));
   }
    Serial.println(F("BH1750 Test begin"));
   lcd.init();  
   lcd.backlight();
   lcd.print("Nhiet do: ");
   lcd.setCursor(0,1);
   lcd.print("Do am: ");
   lcd.createChar(1, degree);
}

void loop() 
{
  uint16_t lux = lightMeter.readLightLevel();     //unsigned short
  Firebase.setInt(firebasedata,"lux",lux);
  Serial.print("Light: ");
  Serial.print(lux);
  Serial.println(" lx");
  delay(1000);
  
  Firebase.getString(firebasedata,"Led_Button");
  Led_Button = firebasedata.stringData();
  if (Led_Button == "ON") { // Kiểm tra nếu trạng thái đèn là ON
     Firebase.setString(firebasedata,"Led_Status","ON");
     Serial.println("Led Turned ON");
     digitalWrite(LED_BUILTIN, LOW); // make bultin led ON
     digitalWrite(led, HIGH); // make external led ON
   } 
  else if (Led_Button == "OFF") { // Kiểm tra nếu trạng thái đèn là OFF
     if(lux>2){
         Firebase.setString(firebasedata,"Led_Status","OFF");
         Serial.println("Led Turned OFF");
         digitalWrite(LED_BUILTIN, HIGH); // make bultin led OFF
         digitalWrite(led, LOW); // make external led OFF
          }
     else{
         Firebase.setString(firebasedata,"Led_Status","ON");
         Serial.println("Led Turned ON");
         digitalWrite(LED_BUILTIN, LOW); // make bultin led ON
         digitalWrite(led, HIGH); // make external led ON
      }
     }
  float h = dht.readHumidity();
  Firebase.setFloat(firebasedata,"Do_Am",h);
  float t = dht.readTemperature();
  Firebase.setFloat(firebasedata,"Nhiet_Do",t);
  if (isnan(t) || isnan(h)){
  } 
  else {
    lcd.setCursor(10,0);
    lcd.print(round(t));
    lcd.print(" ");
    lcd.write(1);
    lcd.print("C");
    lcd.setCursor(10,1);
    lcd.print(round(h));
    lcd.print(" %");    
}
}
