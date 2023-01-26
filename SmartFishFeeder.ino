#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <UniversalTelegramBot.h>
#include <EEPROM.h>
#include <Wire.h>
#include <RTClib.h>
#include <Servo.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal_I2C.h>
RTC_DS3231 rtc;
LiquidCrystal_I2C lcd(0x27, 16, 2);

#define BOTtoken "xxxBotTokenxxx"
#define CHAT_ID "xxxChatIDxxx"
#define TRIG_PIN 34
#define ECHO_PIN 35
#define SOUND_SPEED 0.034
#define SUHU_PIN 4
#define TURBIDITY_PIN 32
#define SERVO_PIN 33

const char* ssid = "FRYZZYS";
const char* password = "1234567890";
unsigned long previousMillis = 0;
unsigned long interval = 30000;
unsigned long currentMillis = millis();
int botRequestDelay = 500;
unsigned long lastTimeBotRan;

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

long duration;
float distance;

OneWire oneWire(SUHU_PIN);
DallasTemperature sensors(&oneWire);
float temperature;

float teg;
float kekeruhan;

Servo myservo;
int pos = 0;

char dataHari[7][10] = {"Minggu", "Senin", "Selasa", "Rabu", "Kamis", "Jumat", "Sabtu"};
String hari;
int tanggal, bulan, tahun, jam, menit, detik;
int jamMakan,menitMakan, kondisi;

int feed_time1_hour;
int feed_time1_min;
int feed_time2_hour;
int feed_time2_min;
String Data[2];
String Data2[5];
String cht;

float fturbidity = kekeruhan;
float fsuhu = temperature;
float A, B;

float udingin[] = {0, 20, 22};
float unormal[] = {20, 25, 30};
float upanas[] = {28, 30, 35};

float ujernih[] = {0, 25, 27};
float usedang[] = {25, 37, 50};
float ukeruh[] = {48, 50, 75};

float outshort = 3;
float outmedium = 2;
float outlong = 1;

float minr[10];
float Rule[10];

void setup() {
  Serial.begin(9600);
  Wire.begin();
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  lcd.init();
  lcd.backlight();
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
//  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print("Connecting to WiFi...\n");
  }
  Serial.println(WiFi.localIP());
  EEPROM.begin(10);
  get_feed_time1();
  get_feed_time2();
  get_last_completed_feeding();
}

void loop() {
  check_feedtime();
  botruns();
  rtcs();
  display_tgl_wkt();
  if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >= interval)){
    reconnect_wifi();
  }
}

float fudingin(){
    if (temperature < udingin[1]){
        return 1;
    }
    else if (temperature >= udingin[1] && temperature <= udingin[2]){
        return (udingin[2] - temperature) / (udingin[2] - udingin[1]);
    }
    else if (temperature > udingin[2]){
        return 0;
    }
}
float funormal(){
    if (temperature < unormal[0]){
        return 0;
    }
    else if (temperature >= unormal[0] && temperature <= unormal[1]){
        return (temperature - unormal[0]) / (unormal[1] - unormal[0]);
    }
    else if (temperature >= unormal[1] && temperature <= unormal[2]){
        return (unormal[2] - temperature) / (unormal[2] - unormal[1]);
    }
    else if (temperature > unormal[2]){
        return 0;
    }
}
float fupanas(){
    if (temperature < upanas[0]){
        return 0;
    }
    else if (temperature >= upanas[0] && temperature <= upanas[1]){
        return (temperature - upanas[0]) / (upanas[1] - upanas[0]);
    }
    else if (temperature > upanas[1]){
        return 1;
    }
}
float fujernih(){
    if (kekeruhan < ujernih[1]){
        return 1;
    }
    else if (kekeruhan >= ujernih[1] && kekeruhan <= ujernih[2]){
        return (ujernih[2] - kekeruhan) / (ujernih[2] - ujernih[1]);
    }
    else if (kekeruhan > ujernih[2]){
        return 0;
    }
}
float fusedang(){
    if (kekeruhan < usedang[0]){
        return 0;
    }
    else if (kekeruhan >= usedang[0] && kekeruhan <= usedang[1]){
        return (kekeruhan - usedang[0]) / (usedang[1] - usedang[0]);
    }
    else if (kekeruhan >= usedang[1] && kekeruhan <= usedang[2]){
        return (usedang[2] - kekeruhan) / (usedang[2] - usedang[1]);
    }
    else if (kekeruhan > usedang[2]){
        return 0;
    }
}
float fukeruh(){
    if (kekeruhan <= ukeruh[0]){
        return 0;
    }
    else if (kekeruhan >= ukeruh[0] && kekeruhan <= ukeruh[1]){
        return (kekeruhan - ukeruh[0]) / (ukeruh[1] - ukeruh[0]);
    }
    else if (kekeruhan >= ukeruh[1]){
        return 1;
    }
}
float Min(float a, float b){
    if (a < b){
        return a;
    }
    else if (b < a){
        return b;
    }
    else{
        return a;
    }
}
void rule(){
    minr[1] = Min(fudingin(), fujernih());
    Rule[1] = outmedium;
    
    minr[2] = Min(fudingin(), fusedang());
    Rule[2] = outlong;

    minr[3] = Min(fudingin(), fukeruh());
    Rule[3] = outlong;
    
    minr[4] = Min(funormal(), fujernih());
    Rule[4] = outshort;

    minr[5] = Min(funormal(), fusedang());
    Rule[5] = outmedium;

    minr[6] = Min(funormal(), fukeruh());
    Rule[6] = outlong;

    minr[7] = Min(fupanas(), fujernih());
    Rule[7] = outshort;

    minr[8] = Min(fupanas(), fusedang());
    Rule[8] = outlong;

    minr[9] = Min(fupanas(), fukeruh());
    Rule[9] = outlong;
}

float defuzzyfikasi()
{
    rule();
    A = 0;
    B = 0;
    for (int i = 1; i <= 9; i++)
    {
        A += Rule[i] * minr[i];
        B += minr[i];
    }
    return A / B;
}

void handleNewMessages(int numNewMessages){
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;
    Serial.println(text);
    cht = chat_id;

    
    int index  = 0;
    char delimiter_tanggal = '#';
    char delimiter_waktu = ':';
    Data[0] = "";
    Data[1] = "";
    Data2[0] = "";
    Data2[1] = "";
    Data2[2] = "";
    Data2[3] = "";
    Data2[4] = "";

    for(int i=0; i < text.length(); i++){
      if(text[i] != delimiter_waktu){
        Data[index] += text[i];    
      }
      else{
        index = index + 1;
        delay(10);
      }
    }
    for(int i=0; i < text.length(); i++){
      if(text[i] != delimiter_tanggal){
        Data2[index] += text[i];                        
      }
      else{
        index = index + 1;
        delay(10);
      }
    }
    if (index == 1){
      Serial.println(kondisi);
      if(kondisi == 1){
        feed_time1_hour   = Data[0].toInt();                 
        feed_time1_min    = Data[1].toInt();
        write_feeding_time1();
        bot.sendMessage(chat_id, "Jadwal pakan 1 telah di perbarui");
        Serial.println("Jadwal pakan 1 telah di perbarui");
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Jadwal 1");
        lcd.setCursor(0,1);
        lcd.print("Berhasil");
        delay(1500);
      }
      if(kondisi == 2){
        feed_time2_hour   = Data[0].toInt();                 
        feed_time2_min    = Data[1].toInt();
        write_feeding_time2();
        bot.sendMessage(chat_id, "Jadwal pakan 2 telah di perbarui");
        Serial.println("Jadwal pakan 2 telah di perbarui");
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Jadwal 2");
        lcd.setCursor(0,1);
        lcd.print("Berhasil");
        delay(1500);  
      }
     }
    if(index == 4){
      int thn = Data2[0].toInt();
      int bln = Data2[1].toInt();
      int hri = Data2[2].toInt();
      int jm = Data2[3].toInt();
      int mnt = Data2[4].toInt();

      rtc.adjust(DateTime(thn, bln, hri, jm, mnt));
      bot.sendMessage(chat_id, "Pembaharuan Tanggal dan Waktu berhasil");
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Waktu & Tanggal");
      lcd.setCursor(0,1);
      lcd.print("Berhasil");
      delay(1500);
    }
    if (text == "/pakan1") {
      bot.sendMessage(chat_id, "Masukan Jam dan menit untuk jadwal pakan ke-1 || cnth: 15:00", "");
      kondisi = 1;
    }
    else if (text == "/pakan2") {
      bot.sendMessage(chat_id, "Masukan Jam dan menit untuk jadwal pakan ke-2 || cnth: 15:00", "");
      kondisi = 2;
    }
    else if (text== "/set_tanggal"){
      bot.sendMessage(chat_id, "Masukan Format tahun#bulan#tanggal#jam#menit");
    }
    else if (text == "/jadwal"){
      String cek_jadwal = "Jadwal Pakan Hari Ini\n";
      cek_jadwal += String() + "Jam Makan 1 : " + feed_time1_hour + " : " + feed_time1_min + "\n";
      cek_jadwal += String() + "Jam Makan 2 : " + feed_time2_hour + " : " + feed_time2_min + "\n";
      bot.sendMessage(chat_id, cek_jadwal);
      String lcd_jadwal1 = String() + feed_time1_hour + " : " + feed_time1_min;
      String lcd_jadwal2 = String() + feed_time2_hour + " : " + feed_time2_min;
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Jadwal Pakan");
      lcd.setCursor(0,1);
      lcd.print(lcd_jadwal1);
      lcd.setCursor(8,1);
      lcd.print(lcd_jadwal2);
      delay(1500);
    }
    else if (text == "/last_feed"){
      String cek_pakan_terakhir = "Pakan Terakhir diberikan pada jam: ";
      cek_pakan_terakhir += String() + jamMakan + " : " + menitMakan + "\n";
      bot.sendMessage(chat_id, cek_pakan_terakhir);
      String lcd_pakan_terakhir = String() + jamMakan + " : " + menitMakan;
      lcd.clear();
      lcd.setCursor(3,0);
      lcd.print("Last Feed");
      lcd.setCursor(2,1);
      lcd.print(lcd_pakan_terakhir);
      delay(1500);
    }
    else if (text == "/feed"){
      manual_feed();
      bot.sendMessage(chat_id, "Pakan telah selesai diberikan secara manual", "");
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Manual Feed Done");
      delay(1500);
    }
    else if (text == "/tanggal"){
      String Waktu = " Tanggal dan Waktu Sekarang \n";
      Waktu += String() + hari + ", " + tanggal + "-" + bulan + "-" + tahun+ " ";
      Waktu += String() + jam + " : " + menit + " : " + detik;
      bot.sendMessage(chat_id, Waktu);
      String lcdtanggal = String() + hari + "," + tanggal + "-" + bulan + "-" + tahun;
      String lcdwaktu = String() + jam + " : " + menit + " : " + detik;
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(lcdtanggal);
      lcd.setCursor(2,1);
      lcd.print(lcdwaktu);
      delay(1500);
    } 
    else if (text == "/tank"){
      ultrasonik();
      if(distance <= 2){
        String jarak = "Jarak: ";
        jarak += String(distance);
        jarak += " CM\n";
        jarak += "Pakan Penuh";
        bot.sendMessage(chat_id, jarak);
      }
      else if(distance >= 2 && distance <= 4.5){
        String jarak = "Jarak: ";
        jarak += String(distance);
        jarak += " CM\n";
        jarak += "Pakan Masih Setengah";
        bot.sendMessage(chat_id, jarak);
      }
      else if(distance >= 4.5 && distance <= 9.5){
        String jarak = "Jarak: ";
        jarak += String(distance);
        jarak += " CM\n";
        jarak += "Pakan Hampir Habis\n";
        jarak += "Segera Isi Pakan";
        bot.sendMessage(chat_id, jarak);
      }
      else if(distance > 9.5){
        String jarak = "Jarak: ";
        jarak += String(distance);
        jarak += " CM\n";
        jarak += "Pakan Habis";
        bot.sendMessage(chat_id, jarak);
      }
    }
    else if (text == "/cek"){
      sensor_value();
      String cek_value = "CEK VALUE SENSOR\n";
      cek_value += String() + "SUHU: " + temperature + "\n";
      cek_value += String() + "TURBIDITY: " + kekeruhan + "\n";
      bot.sendMessage(chat_id, cek_value);
    }
    else if (text == "/options"){
      String keyboardJson = "[[\"/pakan1\",\"/pakan2\",\"/feed\"],[\"/jadwal\",\"/tank\",\"/last_feed\"],[\"/cek\", \"/set_tanggal\",\"/tanggal\"]]";
      bot.sendMessageWithReplyKeyboard(chat_id, "Choose from one of the following options", "", keyboardJson, true);
    }
  }
}

void botruns(){
  if (millis() > lastTimeBotRan + botRequestDelay)  {
  int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

  while (numNewMessages) {
    handleNewMessages(numNewMessages);
    numNewMessages = bot.getUpdates(bot.last_message_received + 1);
   }
   lastTimeBotRan = millis();
  }
}

void reconnect_wifi(){
    Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
}

void display_tgl_wkt(){
  String lcdtanggal = String() + hari + "," + tanggal + "-" + bulan + "-" + tahun;
  String lcdwaktu = String() + jam + " : " + menit + " : " + detik;
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(lcdtanggal);
  lcd.setCursor(2,1);
  lcd.print(lcdwaktu);
  delay(100);
}

void ultrasonik(){
  for (int i=0; i <= 4; i++){
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    duration = pulseIn(ECHO_PIN, HIGH);
    distance = duration * SOUND_SPEED/2;
    delay(100);
  }
}

void suhu(){
  for (int i=0; i <= 4; i++){
    sensors.requestTemperatures(); 
    temperature = sensors.getTempCByIndex(0);
    delay(100);
  }
}

void turbidity(){
  for (int i=0; i <= 4; i++){
    int val = analogRead(TURBIDITY_PIN);
    teg = val * (3.3/4096);
    kekeruhan = 100.00 - (teg/1.60) * 100.00;
    delay(100);
  }
}

void sensor_value(){
  suhu();
  turbidity();
  Serial.print("Suhu: ");
  Serial.println(temperature);
  Serial.print("Turbidity: ");
  Serial.println(kekeruhan);
}

void rtcs(){
  DateTime now = rtc.now();
  hari    = dataHari[now.dayOfTheWeek()]; 
  jam     = now.hour(), DEC;
  menit   = now.minute(), DEC;
  detik   = now.second(), DEC;
  tanggal = now.day(), DEC;
  bulan   = now.month(), DEC;
  tahun   = now.year(), DEC;   
}

void check_feedtime (){
  if (detik == 0){
    if (jam == feed_time1_hour && menit == feed_time1_min){
        Serial.println("First Feeding");
        bot.sendMessage(cht, "First Feeding Done");
        startFeeding();
        jamMakan = feed_time1_hour;
        menitMakan = feed_time1_min;
        EEPROM.write(6, jamMakan);
        EEPROM.write(7, menitMakan);
        EEPROM.commit();
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Pakan 1");
        lcd.setCursor(0,1);
        lcd.print("Berhasil");
        delay(1500);
    }
    else if (jam == feed_time2_hour && menit == feed_time2_min){
        Serial.println("Second Feeding");
        bot.sendMessage(cht, "Second Feeding Done");
        startFeeding();
        jamMakan = feed_time2_hour;
        menitMakan = feed_time2_min;
        EEPROM.write(6, jamMakan);
        EEPROM.write(7, menitMakan);
        EEPROM.commit();
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Pakan 2");
        lcd.setCursor(0,1);
        lcd.print("Berhasil");
        delay(1500);
    }
  }  
}

void startFeeding(){
  sensor_value();
  ultrasonik();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Feeding");
  float keputusan = defuzzyfikasi();
  myservo.attach(SERVO_PIN);
  if (keputusan <= 1){
    for (pos = 0; pos <= 50; pos++){
      myservo.write(pos);
      delay(5);
    }
    for (pos = 50; pos >= 0; pos--){
      myservo.write(pos);
      delay(5);
    }
    Serial.print("Sedikit: ");
    Serial.println(keputusan);
  }
  else if (keputusan >= 1 && keputusan <= 2){
    for (pos = 0; pos <= 55; pos++){
      myservo.write(pos);
      delay(5);
    }
    for (pos = 55; pos >= 0; pos--){
      myservo.write(pos);
      delay(5);
    }
    Serial.print("Sedang: ");
    Serial.println(keputusan);
  }
  else if (keputusan >= 2 && keputusan <= 3){
    for (pos = 0; pos <= 60; pos++){
      myservo.write(pos);
      delay(5);
    }
    for (pos = 60; pos >= 0; pos--){
      myservo.write(pos);
      delay(5);
    }
    Serial.print("Cukup: ");
    Serial.println(keputusan);
  }
  else if (keputusan >= 3){
    for (pos = 0; pos <= 60; pos++){
      myservo.write(pos);
      delay(5);
    }
    for (pos = 60; pos >= 0; pos--){
      myservo.write(pos);
      delay(5);
    }
    Serial.print("Cukup: ");
    Serial.println(keputusan);
  }
  if(distance >= 4.5 && distance <= 9.5){
    String jarak = "Jarak: ";
    jarak += String(distance);
    jarak += " CM\n";
    jarak += "Pakan Hampir Habis\n";
    jarak += "Segera Isi Pakan";
    bot.sendMessage(cht, jarak);
  }
  
  jamMakan = jam;
  menitMakan = menit;
  EEPROM.write(6, jamMakan);
  EEPROM.write(7, menitMakan);
  EEPROM.commit();
  myservo.detach();
  delay(1000);
}

void manual_feed(){
    Serial.println(" Manual Feeding");
    bot.sendMessage(cht, "Manual Feeding");
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Manual Feeding");
    lcd.setCursor(0,1);
    lcd.print("Berhasil");
    startFeeding();
    jamMakan = jam;
    menitMakan = menit;
    EEPROM.write(6, jamMakan);
    EEPROM.write(7, menitMakan);
    EEPROM.commit();
}

void write_feeding_time1(){
  EEPROM.write(0, feed_time1_hour);
  EEPROM.write(1, feed_time1_min);
  EEPROM.commit();
}

void write_feeding_time2() {
  EEPROM.write(2, feed_time2_hour);
  EEPROM.write(3, feed_time2_min);
  EEPROM.commit();
}

void get_feed_time1(){
  feed_time1_hour = EEPROM.read(0);
  if (feed_time1_hour > 23) feed_time1_hour = 0;
  feed_time1_min = EEPROM.read(1);
  if (feed_time1_min > 59) feed_time1_min = 0;
  Serial.println((String) + EEPROM.read(0) + " : " + EEPROM.read(1));
}

void get_feed_time2(){
  feed_time2_hour = EEPROM.read(2);
  if (feed_time2_hour > 23) feed_time2_hour = 0;
  feed_time2_min = EEPROM.read(3);
  if (feed_time2_min > 59) feed_time2_min = 0;
  Serial.println((String) + EEPROM.read(2) + " : " + EEPROM.read(3));
}

void get_last_completed_feeding(){
  jamMakan    = EEPROM.read(6);
  if (jamMakan > 23) jamMakan = 0;
  menitMakan  = EEPROM.read(7);
  if (menitMakan > 59) menitMakan = 0;
  Serial.println((String) + EEPROM.read(6) + " : " + EEPROM.read(7));
}
