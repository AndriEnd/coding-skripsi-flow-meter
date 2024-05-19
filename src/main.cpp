#include <TFT_eSPI.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <WiFi.h>
#include <CTBot.h>

CTBot myBot;
TFT_eSPI tft = TFT_eSPI();

//flow dan led
#define SENSOR 13
#define LED 12

//turby
int pinTurb = 36;
static float V;
static float kekeruhan;
float volt;

const char *WIFI_SSID = "WifiBawaan";
const char *WIFI_PASSWORD = "mifi1234";



//con telegram
String token = "********************";
const int id = 12;
int botRequestDelay = 1700;
unsigned long lastTimeBotRan;

//flow
int interval = 1000;
boolean ledState = false, led = false;
float calibrationFactor = 4.5;
volatile byte pulseCount;
byte pulse1Sec = 0;
float flowRate;
unsigned int flowMilliLitres, kalkulasiAir;
unsigned long totalMilliLitres, totalMilliLitresPrev, counterKalkulasiAir, currentMillis = 0, previousMillis = 0, previousMillis2 = 0, mainTimer = 0, prevPerJam = 0, prevRead = 0, prevTampil = 0, millisLED = 0;
volatile int count;

void IRAM_ATTR pulseCounter()
{
  pulseCount++;
}

void Flow()
{
  count++;
}
WiFiClient net;
void connectToWiFi()
{

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 15)
  {
    delay(500);
    Serial.print(".");
    retries++;
  }

  if (WiFi.status() != WL_CONNECTED) {
    ledState = false;
  } else
  {
    ledState = true;
  }
}


void setup()
{
  tft.begin();
  tft.setRotation(1);
  Serial.begin(115200);
  pinMode(SENSOR, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);
  attachInterrupt(0, Flow, RISING);
  delay(20);

  pulseCount = 0;
  flowRate = 0.0;
  flowMilliLitres = 0;
  totalMilliLitres = 0;
  previousMillis = 0;
  attachInterrupt(digitalPinToInterrupt(SENSOR), pulseCounter, FALLING);

  Serial.println(" mulai telegram");

  connectToWiFi();
  if (WiFi.isConnected() == true)
  {
    Serial.println("sudah terhubung");
    ledState = true;
  }
  else
  {
    connectToWiFi();
    Serial.println("Menghubungkan Ulang");
  }
  //connect wifi
  myBot.wifiConnect(WIFI_SSID, WIFI_PASSWORD);
  //set token
  myBot.setTelegramToken(token);
  //cek tele
  if (myBot.testConnection() )
    Serial.println("koneksi tele berhasil");
  else
    Serial.println ("koneksi tele gagal");


}
void loop()
{

  currentMillis = millis();
  if (currentMillis - previousMillis2 > 1000)
  {
    previousMillis2 = currentMillis;
    mainTimer++;
  }

  //READ
  if (currentMillis - previousMillis > interval)
  {
    pulse1Sec = pulseCount;
    pulseCount = 0;

    flowRate = ((1000.0 / (millis() - previousMillis)) * pulse1Sec) / calibrationFactor;
    previousMillis = millis();

    flowMilliLitres = (flowRate / 20) * 1000;

    totalMilliLitres += flowMilliLitres;
    if (totalMilliLitres != totalMilliLitresPrev)
    {
      counterKalkulasiAir = totalMilliLitres - totalMilliLitresPrev;
      kalkulasiAir = kalkulasiAir + counterKalkulasiAir;
      totalMilliLitresPrev = totalMilliLitres;
    }
  }

  if (mainTimer >= prevPerJam + 86400)
  {
    kalkulasiAir = 0;
    prevPerJam = mainTimer;

  }
  if (mainTimer >= prevPerJam + 3600 )
  {
    flowRate = 0;
    prevPerJam = mainTimer;

  }
  volt = 0;
  for (int i = 0; i < 200; i++)
  {
    volt += ((float)analogRead(pinTurb) / 1023) * 1.25;
  }
  volt = volt;
  int SensorKekeruhan = analogRead(pinTurb);
  V = SensorKekeruhan * (5.0 / 1024);
  kekeruhan = 2.8 + (V * 100.00);

  Serial.print("Voltage     :");
  Serial.print(volt);
  Serial.println("  V");
  Serial.print("Nilai ADC   :");
  Serial.println(SensorKekeruhan);
  Serial.print("kekeruhan   :");
  Serial.print(kekeruhan);
  Serial.println("  NTU");

  if ((SensorKekeruhan >= 0) && (SensorKekeruhan <= 15)) {
    Serial.print("Status Air  :");
    Serial.println("AIR BERSIH");
  }
  else if ((SensorKekeruhan >= 16) && (SensorKekeruhan < 23)) {
    Serial.print("Status Air  :");
    Serial.println("SEDANG");
  }
  else {
    Serial.print("Status Air  :");
    Serial.println("AIR KERUHH");
  }
  delay(2000);
  //display
  tft.setTextSize(2);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  tft.setCursor(0, 00);
  tft.println("==STATUS SENSOR 1==");
  tft.println("____________________");

  if ((SensorKekeruhan >= 0) && (SensorKekeruhan <= 15)) {
    tft.println("STATUS AIR : BERSIH ");
    Serial.println("BERSIH");

  }
  else if ((SensorKekeruhan >= 16) && (SensorKekeruhan < 23)) {
    tft.println("STATUS AIR :SEDANG ");
    Serial.println("SEDANG");
  }
  else {
    tft.println("STATUS AIR : KERUH");
    Serial.println("KERUH");
  }
  tft.print("KEKERUHAN :");
  tft.print(SensorKekeruhan);
  tft.println(" NTU");

  tft.print("DEBIT :");
  tft.print(flowRate);
  tft.println(" L/Min");

  tft.print("AVG :");
  tft.print ((float)kalkulasiAir / 1000);
  tft.println (" L/Hari ");

  tft.print("TOTAL : ");
  tft.print((float)totalMilliLitres / 1000);
  tft.println(" L ");

  if (mainTimer >= prevTampil + 5)
  {
    // DEBIT/MENIT
    Serial.print(" Debit              : ");
    Serial.print(int(flowRate)); // Print the integer part of the variable
    Serial.println(" L/Min ");

    // TOTAL DARI NYALA
    Serial.print(" Total              : ");
    Serial.print(totalMilliLitres);
    Serial.println(" L ");

    // RATA-RATA PERJAM
    Serial.print(" Rata-rata Air/Jam  : ");
    Serial.print(kalkulasiAir);
    Serial.println(" L/Hari");
    prevTampil = mainTimer;
  }

  //telegram

  //flow meter 2
  TBMessage msg;

  if (flowRate  > 3.00)
  {
    myBot.sendMessage (msg.sender.id , "PENYIRAMAN 1 BAIK  ");
  }

  //pesan telegram
  if (myBot.getNewMessage(msg))
  {
    if (msg.text.equalsIgnoreCase("/cek status"))
    {
      Serial.println("pesan2 diterima..");

      if ((SensorKekeruhan >= 0) && (SensorKekeruhan <= 15)) {
        myBot.sendMessage (msg.sender.id , "STATUS AIR : BERSIH ");

      }
      else if ((SensorKekeruhan >= 16) && (SensorKekeruhan < 23)) {
        myBot.sendMessage (msg.sender.id , "STATUS AIR :SEDANG ");
        Serial.println("SEDANG");
      }
      else {
        myBot.sendMessage (msg.sender.id , "STATUS AIR : KERUH");
      }
      //variable pesan kirim
      String balasan;
      balasan =
        (String)"Tingkat Kekeruhan :" + kekeruhan + (String) " NTU \n" +
        (String)"FLOW METER 1 :" + " \n" +
        (String)"Debit :" + (int)flowRate + (String) "L/Min \n" +
        (String)"avg/H : " + (float)kalkulasiAir / 1000 + (String) "L/hari \n" +
        (String)"Total :" + (float)totalMilliLitres / 1000 + (String) "L \n" ;
      myBot.sendMessage(msg.sender.id, balasan);
    }
    // pesan telegram kekeruhan air
    if (msg.text.equalsIgnoreCase("/kondisi air"))
    {
      Serial.print("pesan2 diterima");
      //variabel balasan kondisi air
      String balasan2;
      balasan2 =
        (String)"TINGKAT KEKERUHAN :" + kekeruhan + (String) "NTU \n" +
        (String)"NILAI ADC  : " + SensorKekeruhan + (String)" \n" ;

      if ((SensorKekeruhan >= 0) && (SensorKekeruhan <= 15)) {
        myBot.sendMessage (msg.sender.id , "KONDISI AIR BERSIH ");
      }
      else if ((SensorKekeruhan >= 16) && (SensorKekeruhan < 23)) {
        myBot.sendMessage (msg.sender.id , "SEDANG ");
        Serial.println("SEDANG");
      }
      else {
        myBot.sendMessage (msg.sender.id , "KONDISI AIR KERUH ");
      }

      myBot.sendMessage(msg.sender.id, balasan2);
    }


    if (msg.text.equalsIgnoreCase("/aliran"))
    {
      Serial.print("pesan3 diterima");
      //variabel balasan kondisi flow meter 1
      String balasan3;
      balasan3 =  (String)" == FLOW METER 1 ==" + (String)"\n" +
                  (String)"Debit :" + (int)flowRate + (String) "L/Min \n" +
                  (String)"avg/H : " + (float)kalkulasiAir / 1000 + (String) "L/hari \n" +
                  (String)"Total :" + (float)totalMilliLitres / 1000 + (String) "L \n" ;
      myBot.sendMessage(msg.sender.id, balasan3);
    }
  }

  //ngirim mqtt
  if (WiFi.isConnected() == true)
  {
    {
      Serial.println("Terhubungg cok..");
      ledState = true;
    }

    {
      Serial.print("TIDAK TERHUBUNG JANCOKKKKKK.........");
      ledState = false;
      digitalWrite(LED, HIGH);
      {
        ledState = true;
      }
      {
        Serial.println("Gagal Reconnect MQTT");
        ledState = false;
      }
    }
  }
  else
  {
    Serial.println("Menghubungkan Ulang");
    digitalWrite(LED, HIGH);
    connectToWiFi();
    if (WiFi.isConnected() == true)
    {
      {
        ledState = true;
      }
      {
        ledState = false;
      }
    }
    else
    {
      Serial.println("Wifi Gagal Reconnect");
      ledState = false;
    }
  }
  //LED
  if (ledState == false)
  {
    if (currentMillis - millisLED > 200) {
      led = ! led;
      digitalWrite(LED, led);
      millisLED = currentMillis;
    }
  } else if (ledState == true) {
    if (currentMillis - millisLED > 1500) {
      led = ! led;
      digitalWrite(LED, led);
      millisLED = currentMillis;
    }
    //digitalWrite(pinLED, LOW);
  }
}