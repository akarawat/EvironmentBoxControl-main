/*
  Akarawat Panwilai
  Add to EEPROM
  Rx SoftSerial
  กด * เพื่อเข้า Menu
  กด D เพื่อ Exit/Reboot
  - ตั้งค่าเฉพาะ 1,2,3 ที่เหลือ เป็นทดสอบ Outout ก็พอ
  Output 1 = Fan
  Output 2 = Vale
  Output 3 = Motor
  1.temp_min(offfan)|2.temp_max(onfan)|3.hum_min(offfan)|4.hum_max(onfan)|waterlevel(vale for on/off motor)5.max800(offmotor)|6.min3(onmotor)|water_vale(onevery 7.sec43200=12hr duration on time is 8.3600 sec)
  Default = 25|30|60|80|800|3|43200|3600
  Default = 25|30|60|80|800|3|1830|3600|000000
  P8 Receive Rx, Tx from ESP > Tx
  - อุณหภูมิ
  - ความชื้น
  - ระดับน้ำ (ใช้วิธี ตั้งเวลาเปลี่ยนน้ำทิ้ง ปิด/เปิด อัตโนมัติ)
  - กำหนดให้ Valve ให้เปิดค้างตาม Sec_time โดยขั้นต่ำที่ 60 วินาที

*/
#include <avr/wdt.h>

#include <LiquidCrystal.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
//#include <SoftwareSerial.h>

#include <EEPROM.h>
int address = 0;
String strEEprom;
String eep_tmin = "25";
String eep_tmax = "30";
String eep_hmin = "60";
String eep_hmax = "80";
String eep_watermax = "800";
String eep_watermin = "3";
String eep_onvalve = "1830";
String eep_temsec = "3600";
String eep_curtime = "000000";

const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
/*for Sticker Key*/

char keys[ROWS][COLS] = {
  {'1', '4', '7', '*'},
  {'2', '5', '8', '0'},
  {'3', '6', '9', '#'},
  {'A', 'B', 'C', 'D'}
};

/*for PCB Key*/
/*
  char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
  };
*/
byte rowPins[ROWS] = {5, 4, 3, 2}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {9, 8, 7, 6}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

//------------ LCD ----------------------------------
LiquidCrystal_I2C lcd(0x3F, 20, 4); // Set the LCD address to (0x27, 16, 2) OR (0x3F, 20, 4) line display
#define PIN_VAL    12
#define PIN_MOT    11
#define PIN_FAN    10

//#define PIN_Rx    10
//#define PIN_Tx    99

// Variable
int MnuIS = 5; //ID Main Menu
int SubIS = 0; //ID Sub Menu
int Buzzer = 13; //
int timeLCD = 60;
unsigned long timeCount = 0;
unsigned long timeCountStart = 0;
boolean lcdON = true;
boolean display_LCD_on(boolean i) {
  if (i == true) {
    lcd.backlight();
    return lcdON = i;
  }
  if (i == false) {
    lcd.noBacklight();
    return lcdON = i;
  }
}

//// EEPROM Start
String EEPROM_read(int index, int length) {
  String text = "";
  char ch = 1;

  for (int i = index; (i < (index + length)) && ch; ++i) {
    if (ch = EEPROM.read(i)) {
      text.concat(ch);
    }
  }
  return text;
}

int EEPROM_write(int index, String text) {
  for (int i = index; i < text.length() + index; ++i) {
    EEPROM.write(i, text[i - index]);
  }
  EEPROM.write(index + text.length(), 0);
  return text.length() + 1;
}

//SoftwareSerial mySerial(PIN_Rx, PIN_Tx);

String inData;
int incomingByte = 0;
String eepStr = "";
String getKb = "";
bool setting = false;
bool flgsave = false;
String curMode = "";

//Clock
int starttime;
int activetime;
int prevoustime = 0;
int hours = 0;
int mins = 0;
int secs = 0;
String txtTime;

// Output Flag
bool valve = false;
bool motor = false;
bool fan = false;

void setup() {
  // 9600, 115200
  Serial.begin(9600);
  //  mySerial.begin(115200);
  while (!Serial);
  //--#Serial.println("Serial conection started.");
  delay(100);

  starttime = millis() / 1000;

  strEEprom = EEPROM_read(address, 34);
  if (strEEprom.length() == 0) {
    eep_tmin = "25";
    eep_tmax = "30";
    eep_hmin = "60";
    eep_hmax = "80";
    eep_watermax = "800";
    eep_watermin = "3";
    eep_onvalve = "1830";
    eep_temsec = "3600";
    eep_curtime = "000000";
    hours = 0;
    mins = 0;
    secs = 0;
  } else {
    //--#Serial.println(strEEprom);
    eep_tmin = getValue(String(strEEprom), '|', 0);
    eep_tmax = getValue(String(strEEprom), '|', 1);
    eep_hmin = getValue(String(strEEprom), '|', 2);
    eep_hmax = getValue(String(strEEprom), '|', 3);
    eep_watermax = getValue(String(strEEprom), '|', 4);
    eep_watermin = getValue(String(strEEprom), '|', 5);
    eep_onvalve = getValue(String(strEEprom), '|', 6);
    eep_temsec = getValue(String(strEEprom), '|', 7);
    eep_curtime = getValue(String(strEEprom), '|', 8);

    hours = eep_curtime.substring(0, 2).toInt();
    mins = eep_curtime.substring(2, 4).toInt();
    secs = eep_curtime.substring(4, 6).toInt();
  }
  pinMode(Buzzer, OUTPUT); digitalWrite(Buzzer, HIGH);
  pinMode(PIN_VAL, OUTPUT);
  pinMode(PIN_MOT, OUTPUT);
  pinMode(PIN_FAN, OUTPUT);

  lcd.init();
  lcd.clear();
  lcd.backlight();
  //  lcd.setCursor(2, 1); lcd.print("EEPROM LOADED"); delay(500);
  //  lcd.setCursor(2, 2); lcd.print(strEEprom);
  //  delay(5000);
  //lcd.clear();
  //--->lcd.noBacklight();
  delay(500);
  digitalWrite(Buzzer, LOW);

  DESKTOP();
}

String MasKey = "";
//ความยาว A=11, B=5, C=16
int lenCfg = 0;
void MainMenu(char Key) {
  if (Key == 'A') {
    MnuIS = 1;
    lenCfg = 12;
    curMode = "A";
    MENUPA();
    getKb = "";
    setting = true;
  }

  if (Key == 'B') {
    MnuIS = 2;
    lenCfg = 6;
    curMode = "B";
    MENUPB();
    getKb = "";
    setting = true;
  }
  if (Key == 'C') {
    MnuIS = 3;
    lenCfg = 17;
    curMode = "C";
    MENUPC();
    getKb = "";
    setting = true;
  }
  if (Key == 'D' && flgsave) { // Save
    MnuIS = 4;
    MENUPD();
    getKb = "";
    setting = false;
    flgsave = false;
    curMode = "SAVE";
  }
  if (Key == '*') {
    MnuIS = 5;
    DESKTOP();
    getKb = "";
    setting = false;
    flgsave = false;
  }

  if (Key != 'A' || Key != 'B' || Key != 'C')  {
    if ((Key != '*' || Key != 'D') and setting) {
      //--#Serial.println(getKb.length());
      if (getKb.length() < lenCfg) {
        getKb += String(Key);
        lcd.setCursor(1, 2); lcd.print(getKb);
        if (getKb.length() == lenCfg) {
          flgsave = true;
        } else {
          flgsave = false;
        }
      }

    }
  }

}

void loop() {
  // Keyboard Transection
  char Key;
  Key = NO_KEY;
  while (Key == NO_KEY) {

    if (timeCountStart > millis()) {
      timeCountStart = 0;
    }
    if ((timeCount < millis() / 1000) && (lcdON == 1)) {
      timeCountStart++;
    }
    if (timeCountStart > timeLCD && timeLCD != 0) {
      timeCountStart = 0;
      display_LCD_on(false); // Trun off backlight
    }
    timeCount = millis() / 1000;
    /// End of LCD Timeout

    Key = keypad.getKey(); // เก็บค่าของปุ่มที่กดมาไว้ที่ตัวแปร Key
    MasKey = Key;

    // Clock Start
    GetClock();
    // Clock End

    // ใช้ NO_KEY ของ KeyPad ดักจับเท่านั้น mySerial
    while (Serial.available()) {
      //        inData = Serial.readString();
      inData = Serial.readStringUntil('\r\n');
    }
    if (inData.length()) {
      //mySerial.write(String(inData));
      //--#Serial.println(String(inData));
      //--#Serial.println(inData.length()); // อ่าน Array ที่ 1
      String strMain = getValue(String(inData), '|', 1);
      String humd = getValue(String(strMain), ',', 0);
      String temp = getValue(String(strMain), ',', 1);
      String water = getValue(String(strMain), ',', 2);
      lcd.clear();
      //lcd.setCursor(0, 0); lcd.print("VRU.ME 62 "+ txtTime);
      lcd.setCursor(0, 1); lcd.print("TEM=" + temp + " HUM=" + humd);
      lcd.setCursor(0, 2); lcd.print("WATER=" + water + " OnV=" + eep_onvalve.substring(0, 2) + ":" + eep_onvalve.substring(2, 4));
      String flgValve;
      String flgMotor;
      String flgFan;

      if (valve) {
        flgValve = "V:ON ";
        digitalWrite(PIN_VAL, LOW);
      } else {
        flgValve = "V:Off ";
        digitalWrite(PIN_VAL, HIGH);
      }

      if (water.toInt() <= eep_watermin.toInt()) {
        flgMotor = "M:On ";
        digitalWrite(PIN_MOT, LOW);
      } else {
        flgMotor = "M:Off ";
      }

      if (water.toInt() >= eep_watermax.toInt()) {
        flgMotor = "M:Off ";
        digitalWrite(PIN_MOT, HIGH);
      }

      if ((temp.toInt() <= eep_tmin.toInt()) || (humd.toInt() <= eep_hmin.toInt())) {
        flgFan = "F:Off";
        digitalWrite(PIN_FAN, HIGH);
      }
      if ((temp.toInt() != "" && humd.toInt() != "") && (temp.toInt() >= eep_tmax.toInt()) || (humd.toInt() >= eep_hmax.toInt())) {
        flgFan = "F:On";
        digitalWrite(PIN_FAN, LOW);
      }

      lcd.setCursor(1, 3); lcd.print(flgValve + flgMotor + flgFan);
      inData = "";
    }


  }

  timeCountStart = 0;
  display_LCD_on(true);

  //--#Serial.println(Key);
  if (MasKey != "") {

    //--#Serial.println("=>" + MasKey);
    digitalWrite(Buzzer, HIGH);
    delay(10);
    digitalWrite(Buzzer, LOW);

    if (MasKey == "*") {
      MnuIS = 5;
      setting = false;
    }
  }

  switch (MnuIS) {
    case 1 : //Group A
      //setting = true;
      MainMenu(Key);
      break;
    case 2 : //Group B
      //setting = true;
      MainMenu(Key);
      break;
    case 3 : //Group C
      //setting = true;
      MainMenu(Key);
      break;
    case 4 : //Group D
      //setting = false;
      MainMenu(Key);
      break;
    case 5 : //DESKTOP Group *
      //setting = false;
      MainMenu(Key);
      break;
    default:
      //setting = false;
      MnuIS = 5;
      break;
  }

}

/// Sub Program
void Beep() {
  digitalWrite(Buzzer, LOW); delay(10); digitalWrite(Buzzer, HIGH);
}
void printFrame() {
  lcd.clear();
  lcd.setCursor(1, 0); lcd.print("------------------");
  lcd.setCursor(1, 3); lcd.print("------------------");
}
void ShowRow1(char X, char Y) {
  lcd.setCursor(1, 1);
  lcd.print("MENU A,B,C, HOME=* ");
}
void ShowRow2(char X, char Y) {
  lcd.setCursor(2, 2);
}
// Main Menu
void DESKTOP() {
  lcd.clear();
  //lcd.setCursor(0, 0); lcd.print("VRU.ME 62 " + txtTime);
  lcd.setCursor(0, 1); lcd.print("CONFIG: " + eep_tmin + "," + eep_tmax + "," + eep_hmin + "," + eep_hmax);
  lcd.setCursor(0, 2); lcd.print(" " + eep_watermax + "," + eep_watermin + "," + eep_onvalve + "," + eep_temsec);
  lcd.setCursor(1, 3); lcd.print("MENU A,B,C, HOME=* ");
  getKb = "";
  setting = false;
}
void MENUPA() {
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("A: Temp/Hunidity");
  lcd.setCursor(0, 1); lcd.print(" Ex. 25#30#60#80 ");
  lcd.setCursor(0, 3); lcd.print(" HOME * / SAVE D");
}
void MENUPB() {
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("B: Water Level");
  lcd.setCursor(0, 1); lcd.print(" Ex. 800#3 ");
  lcd.setCursor(0, 3); lcd.print(" HOME * / SAVE D");
}
void MENUPC() {
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("C: Valve ON-OFF Sec");
  lcd.setCursor(0, 1); lcd.print(" Ex. 1830#3600#183000 ");
  lcd.setCursor(0, 3); lcd.print(" HOME * / SAVE D");
}
void MENUPD() {
  // START Save data to EEPROM
  //  eep_tmin, eepStr
  //--#Serial.println(flgsave);
  //--#Serial.println("SAVE >> " + getKb);
  if (flgsave) {
    String txt = getValue(String(getKb), '#', 0);
    if (curMode == "A") {
      eep_tmin = txt.substring(1, txt.length());
      eep_tmax = getValue(String(getKb), '#', 1);
      eep_hmin = getValue(String(getKb), '#', 2);
      eep_hmax = getValue(String(getKb), '#', 3);
    }
    if (curMode == "B") {
      eep_watermax = txt.substring(1, txt.length());
      eep_watermin = getValue(String(getKb), '#', 1);
    }
    if (curMode == "C") {
      eep_onvalve = txt.substring(1, txt.length());
      eep_temsec = getValue(String(getKb), '#', 1);
      eep_curtime = getValue(String(getKb), '#', 2);
    }

    eepStr = eep_tmin + "|" + eep_tmax + "|" + eep_hmin + "|" + eep_hmax + "|" +
             eep_watermax + "|" + eep_watermin + "|" + eep_onvalve + "|" + eep_temsec + "|" + eep_curtime;

    //--#Serial.println("EEPROM >> " + eepStr);
    EEPROM_write(0, eepStr);
  }

  // End EEPROM
  getKb = "";
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("D: Save");
  lcd.setCursor(0, 1); lcd.print("");
  lcd.setCursor(0, 2); lcd.print("");
  lcd.setCursor(0, 3); lcd.print(" Wait for reboot...");
  delay(1000);
  software_Reboot();
}

void software_Reboot()
{
  wdt_enable(WDTO_15MS);
  while (1)
  {
  }
}

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

int chkCount = 0;
bool chkFlgMin;
String chkTxt = "";
void GetClock() {
  activetime = (millis() / 1000) - starttime;
  if (prevoustime < activetime)
  {
    secs++;
    prevoustime = activetime;
  }
  if (secs > 59)
  {
    mins++;
    secs = 0;
  }
  if (mins > 59)
  {
    hours++;
    mins = 0;
  }
  if (hours > 23)
  {
    hours = 0;
  }

  String hh;
  if (hours < 10)
  {
    hh = "0" + String(hours);
  }
  else
  {
    hh = String(hours);
  }
  String mm;
  if (mins < 10)
  {
    mm = "0" + String(mins);
  }
  else
  {
    mm = String(mins);
  }
  String ss;
  if (secs < 10)
  {
    ss = "0" + String(secs);
  }
  else
  {
    ss = String(secs);
  }
  txtTime = hh + ":" + mm + ":" + ss;
  if (!setting) {
    lcd.setCursor(0, 0); lcd.print("VRU.ME.R62 " + txtTime);

    //ตั้งเวลาเปิดปิดน้ำ
    String txtNow = hh + mm;
    String txtNowss = hh + mm + ss;

    /* //Flow เดิมทำงาน Fix 1 นาที
      if (txtNow == eep_onvalve) {
      valve = true;
      } else {
      valve = false;
      }
    */

    if (txtNowss != chkTxt) { // ให้ตรวจสอบเฉพาะในวินาทีนั้นๆเท่านั้น ไม่งั้นจะทำงานตาม Clock ของ Crystal
      chkTxt = txtNowss;
      //-->Serial.println(txtTime);

      // ปิดเมื่อถึงเวลาใน EEPROM
      if (txtNow == eep_onvalve) {
        chkFlgMin = true; //กำหนดให้เริ่มเปิด
        //-->Serial.println("Set true");
      }
      if (chkFlgMin) {
        if (chkCount < eep_temsec.toInt()) {
          chkCount++; // นับไปจนกว่าจะครบวินาที
          valve = true;
          //-->Serial.println("CountV : " + String(chkCount));
        } else { // เมื่อครบทำการรีเซตและปิด
          chkCount = 0;
          valve = false;
          chkFlgMin = false;
          //-->Serial.println("CountV : " + String(chkCount));
        }
      }

    }

  }
}
