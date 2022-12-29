
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <SPI.h>
#include <Wire.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <FirebaseESP8266.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>
FirebaseData firebasedata;

//Variables
int i = 0;
int statusCode;
SoftwareSerial s(13,15);                                    //SoftwareSerial s(RX,TX)
LiquidCrystal_I2C lcd (0x27,16,2);                          //Khởi tạo LCD I2C
#define FIREBASE_HOST  "smarthome-d9efb-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH  "2asbijf10Az16fFyBMEc9fiUJtWqT46B9OIE0Iw5"
constexpr uint8_t RST_PIN = D3;                             // RST RFID
constexpr uint8_t SS_PIN = D4;                              // SDA RFID
MFRC522 rfid(SS_PIN, RST_PIN); 
MFRC522::MIFARE_Key key;
String tag;
const char* ssid = "Quan";
const char* passphrase = "quanprovip";
String st;
String content;
bool testWifi(void);
void launchWeb(void);
void setupAP(void);
ESP8266WebServer server(80);

void setup()
{
  Wire.begin(D2,D1);                                      //2 chân kết nối với SDA và SCK của LCD I2C
  lcd.init();         
  lcd.backlight();
  lcd.clear();
  s.begin(9600);
  Serial.begin(9600); 
  Serial.println();
  WiFi.mode(WIFI_AP);
  lcd.setCursor(0,1);
  lcd.print("Disconnecting");
  delay(500);
  Serial.println("Disconnecting current wifi connection");
  WiFi.disconnect();
  EEPROM.begin(512);                                      //Initialasing EEPROM
  delay(10);
  pinMode(2, OUTPUT);
  pinMode(0, OUTPUT);
  Serial.println();
  Serial.println();
  Serial.println("Startup");
  lcd.clear();
  lcd.print("Startup");
  delay(500);
//---------------------------------------- Read eeprom for ssid and pass
  Serial.println("Reading EEPROM ssid");
  lcd.clear();
  lcd.print("Getting data");
  delay(500);
  String esid;
  for (int i = 0; i < 32; ++i)
  {
    esid += char(EEPROM.read(i));
  }
  Serial.println();
  Serial.print("SSID: ");
  Serial.println(esid);
  Serial.println("Reading EEPROM pass");
  String epass;
  for (int i = 32; i < 96; ++i)
  {
    epass += char(EEPROM.read(i));
  }
  Serial.print("PASS: ");
  //Serial.println(epass);
  WiFi.begin(esid.c_str(), epass.c_str());
  
  Firebase.begin(FIREBASE_HOST,FIREBASE_AUTH);
  SPI.begin();                                                    //Khởi tạo giao thức SPI
  rfid.PCD_Init();                                                //Khởi tạo MRC522
  if (testWifi())                                                 //Kiểm tra wifi
  { 
    Serial.println("Succesfully Connected!!!");
    lcd.clear();
    lcd.print("Successfully");
    delay(2000);
    return;
  }
  else
  {
    Serial.println("Turning the HotSpot On");
    lcd.print("Failed");
    lcd.setCursor(0,1);
    lcd.clear();
    lcd.print("Hotspot on");
    delay(500);
    launchWeb();
    setupAP();// Setup HotSpot
  }
  Serial.println();
  Serial.println("Waiting.");
  while ((WiFi.status() != WL_CONNECTED))
  {
    Serial.print(".");
    delay(100);
    server.handleClient();
  }
  
}
void loop() {           
  String isAdd;
  String opening;
  String isChangingPass;
  Firebase.getString(firebasedata,"isChange");  // Khai báo các biến và lấy giá trị nó trong database
  isChangingPass = firebasedata.stringData();
  Firebase.getString(firebasedata,"isAdding");   
  isAdd = firebasedata.stringData();                       
  Firebase.getString(firebasedata,"open");
  opening = firebasedata.stringData();
  if (isChangingPass == "1")                                              //Thay đổi mật khẩu
  {
    Serial.println("change successfull");
  }
  if (opening == "1")                                                     // Mở cửa
  {
    lcd.setCursor(0,0);
    lcd.print("Acccess granted");
    s.write("1");                                                         // Gửi tín hiệu tới Arduino
    Serial.println("1");
    delay(1000);
    Firebase.setString(firebasedata,"open","0");
  }
  if (isAdd == "1")                                                       // Thêm 1 thẻ mới
  {
    lcd.setCursor(0,0);                                                   
    lcd.print("Insert Card");
    if ( ! rfid.PICC_IsNewCardPresent())                  
    return;
    if (rfid.PICC_ReadCardSerial()) {
    for (byte i = 0; i < 4; i++) {
      tag += rfid.uid.uidByte[i];
    }
    String nname;
    Firebase.getString(firebasedata,"newname");
    nname =  firebasedata.stringData();
    Firebase.setString(firebasedata,tag,nname);
    Firebase.setString(firebasedata,"isAdding","0");
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Successfully");
    tag = "";
  }
  }
    lcd.clear();
    lcd.print("Working!");
    
    String personStatus;
    if ( ! rfid.PICC_IsNewCardPresent())
    return;
  if (rfid.PICC_ReadCardSerial()) {
    for (byte i = 0; i < 4; i++) {
      tag += rfid.uid.uidByte[i];
    }
    Firebase.getString(firebasedata,tag);
    personStatus = firebasedata.stringData();
    if (personStatus.length()!=0) {
      s.write("1");                                       // gửi 1 tới arduino
      Serial.println("1");
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(personStatus);
      lcd.setCursor(0,1);
      lcd.print("Da vao nha");
      delay(1500);
      Firebase.setString(firebasedata,"goHome",personStatus);
    } else {
      Serial.println("0");
      lcd.clear();
      lcd.print("Access Denied!");
      delay(2000);
    }
    tag = "";
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }
  
}
//------------------------ Fuctions used for WiFi credentials saving and connecting to it which you do not need to change
bool testWifi(void)
{
  int c = 0;
  Serial.println("Waiting for Wifi to connect");
  while ( c < 20 ) {
    if (WiFi.status() == WL_CONNECTED)
    {
      return true;
    }
    delay(500);
    Serial.print("*");
    c++;
  }
  Serial.println("");
  Serial.println("Connect timed out, opening AP");
  return false;
}
void launchWeb()
{
  Serial.println("");
  if (WiFi.status() == WL_CONNECTED)
    Serial.println("WiFi connected");
  server.begin();
  Serial.println("Server started");
}
void setupAP(void)
{
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0)
    Serial.println("no networks found");
  else
  {
    Serial.println("networks found");
    for (int i = 0; i < n; ++i)
    {
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
      delay(10);
    }
  }
  Serial.println("");
  st = "<ol>";
  for (int i = 0; i < n; ++i)
  {
    st += "<li>";
    st += WiFi.SSID(i);
    st += " (";
    st += WiFi.RSSI(i);
    st += ")";
    st += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*";
    st += "</li>";
  }
  st += "</ol>";
  delay(100);
  WiFi.softAP("ESP Cua QUAN", "");
  Serial.println("Initializing_softap_for_wifi credentials_modification");
  launchWeb();
  Serial.println("over");
}
void createWebServer()
{
  {
    server.on("/", []() {
      IPAddress ip = WiFi.softAPIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      content = "<!DOCTYPE HTML>\r\n<html>Welcome to Wifi Credentials Update page";
      content += "<form action=\"/scan\" method=\"POST\"><input type=\"submit\" value=\"scan\"></form>";
      content += ipStr;
      content += "<p>";
      content += st;
      content += "</p><form method='get' action='setting'><label>SSID: </label><input name='ssid' length=32><input name='pass' length=64><input type='submit'></form>";
      content += "</html>";
      server.send(200, "text/html", content);
    });
    server.on("/scan", []() {
      //setupAP();
      IPAddress ip = WiFi.softAPIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      content = "<!DOCTYPE HTML>\r\n<html>go back";
      server.send(200, "text/html", content);
    });
    server.on("/setting", []() {
      String qsid = server.arg("ssid");
      String qpass = server.arg("pass");
      if (qsid.length() > 0 && qpass.length() > 0) {
        Serial.println("clearing eeprom");
        for (int i = 0; i < 96; ++i) {
          EEPROM.write(i, 0);
        }
        Serial.println(qsid);
        Serial.println("");
        Serial.println(qpass);
        Serial.println("");
        Serial.println("writing eeprom ssid:");
        for (int i = 0; i < qsid.length(); ++i)
        {
          EEPROM.write(i, qsid[i]);
          Serial.print("Wrote: ");
          Serial.println(qsid[i]);
        }
        Serial.println("writing eeprom pass:");
        for (int i = 0; i < qpass.length(); ++i)
        {
          EEPROM.write(32 + i, qpass[i]);
          Serial.print("Wrote: ");
          Serial.println(qpass[i]);
        }
        EEPROM.commit();
        content = "{\"Success\":\"saved to eeprom... reset to boot into new wifi\"}";
        statusCode = 200;
        ESP.reset();
      } else {
        content = "{\"Error\":\"404 not found\"}";
        statusCode = 404;
        Serial.println("Sending 404");
      }
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(statusCode, "application/json", content);
    });
  }
}
