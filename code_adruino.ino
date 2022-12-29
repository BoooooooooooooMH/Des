#include <Keypad.h>                                         // Khai báo thư viện sử dụng
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <EEPROM.h>
#define pass_length 6                                       // Khai báo độ dài của password
const byte rows = 4;                                        // Khai báo số hàng, cột cho keypad 3x4
const byte cols = 3;                                    
char initial_password[pass_length];                         // Mật khẩu khởi tạo
char password[pass_length];                                 // Mật khẩu hiện tại
char new_password[pass_length];                             // Mật khẩu mới
char pass_check[pass_length];                               // Kiểm tra mật khẩu mới
char getkey = 0;                                              
int isOpen = 0;                                             // Trạng thái của cửa
int i = 0;
int sta;                                                    // Đọc dữ liệu từ Ir1
bool first = false;   
int sta1;                                                   // Đọc dữ liệu từ Ir2
char hexa[rows][cols] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};
byte rowPin[rows] = {13, 12, 11, 10};                       // Chân dùng đề điều khiển keypad
byte colPin[cols] = {7, 6, 5};
LiquidCrystal_I2C lcd (0x27, 16, 2);                      
Keypad key = Keypad(makeKeymap(hexa), rowPin, colPin, rows, cols);
int in1 = 9;                                                // Chân điều khiển động cơ
int in2 = 8;
int irSensor = 2;                                           // Cảm biến hồng ngoại ngoài cửa
int irSensor1 = 3;                                          // Cảm biến hồng ngoại trong cửa
void setup()
{
    lcd.init();
    lcd.backlight();
    lcd.clear();
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode (irSensor, INPUT);
    pinMode (irSensor1, INPUT);
    pinMode(in1, OUTPUT);
    pinMode(in2, OUTPUT);
    attachInterrupt(1,hang, LOW);                           // Ngắt 1 bằng hàm hang
    Serial.begin(9600);
    lcd.setCursor(0, 0);
    lcd.print("Enter Password:");
    lcd.setCursor(0,1);
    initialpassword();                                      // Khởi tạo mật khẩu ban đầu = 123456
}
void hang()                                                 // Interupt được thực hiện khi người ở trong nhà muốn ra ngoài
{
  if (!first)                                               // first = flase
  {
    digitalWrite(in1, LOW);                                 // Mở cửa
    digitalWrite(in2, HIGH);
    delay(60000);
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
    first = true;                                           // first = true
  }
  sta1 = digitalRead(irSensor1);                            // sta1 đọc tín hiệu từ cảm biến ngoài cửa
  if (sta1 == 0)                                            // Người vẫn ở trong -- Không làm gì
  {   
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
  }
  if (sta1 == 1)                                            // Người đã ra ngoài -- Đóng cửa
  {
    delay(700000);
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
    delay(60000);
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
    first = false;
  }
}
void loop()
{
  digitalWrite(LED_BUILTIN, LOW);
  if (isOpen == 1)                                          // Khi cửa đang mở
  {
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
    sta = digitalRead(irSensor);                            // Đọc tín hiệu từ cảm biến ở ngoài cửa
    if (sta == 0)                                           // Nếu người dùng vẫn ở ngoài cửa --> Không làm gì
    {
      detachInterrupt(1);
      digitalWrite(in1, LOW);
      digitalWrite(in2, LOW);
    }
    if (sta == 1)                                           // Nếu người dùng đã vô nhà --> Đóng cửa
    {
      detachInterrupt(1);                                   
      delay(2000);
      Serial.println(isOpen);                     
      digitalWrite(in1, HIGH);
      digitalWrite(in2, LOW);
      delay(280);
      digitalWrite(in1, LOW);
      digitalWrite(in2, LOW);
      attachInterrupt(1, hang, LOW);                        // Khi hệ thống ở trạng thái đóng cửa --> bật interupt1
      isOpen = 0;                                           // Gán cho biến mở cửa là không
    }
  }
  else if (isOpen == 0)                                     // Khi cửa đang đóng
  { 
    if(Serial.available())                                  // Nếu nhận được tín hiệu từ ESP8266 là RFID được quẹt thành công
    {
      char a = Serial.read();
      if (a == 49)
    {
      Serial.println("1");
      digitalWrite(in1, LOW);                               // Mở cửa gắn cho biến mở cửa = 1 và xử lý giống t/h trên
      digitalWrite(in2, HIGH);
      delay(225);
      digitalWrite(in1, LOW);
      digitalWrite(in2, LOW);
      isOpen = 1;                                         
    }
    }
  }
  getkey = key.getKey();                                    // Mở cửa bằng keypad 3x4
  if (getkey == '#')                                        // Nếu phím nhấn là phím # thì hệ thống sẽ đổi mật khẩu bằng hàm change()
    change();
  if (getkey)                                               // Nếu bấm các phím khác
  {
     password[i++]=getkey;                                  // Nhập kí tự từ bàn phím sao cho đủ độ dài của mật khẩu đã được khai báo từ trước 
     //lcd.print(getkey);
     lcd.print("*");
   }
  if (i == pass_length)                                     // Khi đã nhập đủ mật khẩu
   {
     delay(1000);                                         
     for(int j=0  ; j<6 ; j++)
     initial_password[j]= EEPROM.read(j);                   // Đọc mật khẩu đã lưu trong EEPROM
     if (!(strncmp(password, initial_password,6)))          /* So sánh chuỗi vừa nhập và mật khẩu nếu khớp thì in ra thông báo
     {                                                      và điều khiển motor để mở cửa */
        lcd.clear();
        lcd.print("Pass Accepted");           
        lcd.setCursor(0,1);
        lcd.print("Open!!");
        digitalWrite(in1, LOW);
        digitalWrite(in2, HIGH);
        delay(225);
        digitalWrite(in1, LOW);
        digitalWrite(in2, LOW);
        isOpen = 1;                                         // Gắn cho trạng thái là đang mở cửa
        delay(1000);
        lcd.clear();
        lcd.print("Enter Password:");
        lcd.setCursor(0,1);
        i=0;
     }
     else                                                   // Nếu nhập sai thì y/c nhập lại
     {
        digitalWrite(LED_BUILTIN, HIGH);   
        delay(1000);                       
        digitalWrite(LED_BUILTIN, LOW);
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Wrong Password");
        delay(1000);
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Try again");
        delay(1000);
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Enter Password");
        lcd.setCursor(0,1);
        i=0;
     }
   }

void change()                                                 // Hàm thay đổi mật khẩu cửa
{
    int j = 0;
    lcd.clear();
    lcd.print("Current Password");
    lcd.setCursor(0,1);
  while (j < 6)
  {
    char getkey = key.getKey();
    if (getkey)
    {
      password[j++] = getkey;
      lcd.print("*");
    }
    getkey = 0;
  }
  delay(500);
  if((strncmp(password, initial_password, 6)))
  {
    lcd.clear();
    lcd.print("Wrong Password");
    lcd.setCursor(0, 1);
    lcd.print("Try Again");
    delay(1000);
  }
  else
  {
    j = 0;
    int k = 0;
    lcd.clear();
    lcd.print("New Password:");
    lcd.setCursor(0, 1);
    while (j < 6)
    {
      char new_key = key.getKey();
      if (new_key)
      {
        new_password[j] = new_key;
        lcd.print("*");
        EEPROM.write(j, new_key);
        j++;
      }
    }
    delay(1000);
    lcd.clear();
    lcd.print("Confirm Passworld");
    lcd.setCursor(0,1);
    while(k < 6)
    {
      char key_check = key.getKey();
      if (key_check)
      {
        pass_check[k] = key_check;
        lcd.print("*");
        k++;
      }
    }
    if (!(strncmp(new_password, pass_check ,6)))
    {
       delay(1000);
       lcd.clear();
       lcd.print("Pass Changed!!");
       delay(1000);
    }  
    else
    {
       lcd.clear();
       lcd.print("Wrong Password");
       lcd.setCursor(0, 1);
       lcd.print("Try Again");
       delay(1000);
    } 
  }
  lcd.clear();
  lcd.print("Enter Password");
  lcd.setCursor(0, 1);
  getkey = 0;
}

void initialpassword() {                                          // Hàm khởi tạo mật khẩu
  for (int j = 0 ; j < 6 ; j++)
  {
    EEPROM.write(j, j + 49);
  }
  for (int j = 0 ; j < 6 ; j++)
  {
    initial_password[j] = EEPROM.read(j);
  }
}
