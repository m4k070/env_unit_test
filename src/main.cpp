#include <M5Stack.h>
#include "UNIT_ENV.h"
#include <WiFi.h>
#include <WiFiServer.h>
#include <WiFiAP.h>
#include <string>

#define WIDTH 320

SHT3X sht30;
QMP6988 qmp6988;

float tmp = 0.0;
float hum = 0.0;
float pressure = 0.0;

const char *ssid = "UNIT_ENV";
const char *password = "UNIT_ENV";
const int interval = ((24 * 60 * 60) / WIDTH) * 1000;

int cnt = 0;
float tmp_list[WIDTH];
float hum_list[WIDTH];
float prs_list[WIDTH];

void setup_wifi() {
  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
}

WiFiServer* server;
void setupServer()
{
  server = new WiFiServer(80);
  server->begin();
}

void fetchSensorData(void *arg)
{
  while (1)
  {
    pressure = qmp6988.calcPressure();
    if (sht30.get() == 0)
    {                       // Obtain the data of shT30.
      tmp = sht30.cTemp;    // Store the temperature obtained from ShT30.
      hum = sht30.humidity; // Store the humidity obtained from the SHT30.
    }
    else
    {
      tmp = 0, hum = 0;
    }

    float cpu_cel = temperatureRead();

    tmp_list[cnt] = tmp;
    hum_list[cnt] = hum;
    prs_list[cnt] = pressure;
    cnt = (cnt + 1) % WIDTH;

    delay(interval); // センサからの情報取得は4.5分毎
  }
}

void displayEnv()
{
  M5.lcd.fillRect(0, 0, 160, 30, TFT_BLACK); // Fill the screen with black (to clear the screen).
  M5.lcd.setCursor(0, 0);
  M5.Lcd.printf("Temp:     %2.1f C \r\nHumi:     %2.1f %% \r\nPressure:%4.0f hPa\r\nBattery:  %3d %%", tmp, hum, pressure / 100, M5.Power.getBatteryLevel());
}

void drawGraph()
{
  M5.lcd.fillRect(0, 120, 320, 120, TFT_LIGHTGREY);
  for (int i = 0; i < WIDTH; i++) {
    //Serial.println(tmp_list[i]);
    float dpt = 120.0 / 50;
    float raw_t = tmp_list[(cnt + i) % WIDTH];
    float t = raw_t + 10.0;
    int y = 240 - (int)roundf(t * dpt);
    uint32_t color = TFT_GREEN;
    if (raw_t > 30) {
      color = TFT_RED;
    }
    else if (raw_t > 25) {
      color = TFT_ORANGE;
    }
    else if (raw_t < 10) {
      color = TFT_BLUE;
    }
    else if (raw_t < 0) {
      color = TFT_WHITE;
    }
    M5.lcd.drawPixel(i, y, color);
  }
}

void processRequest(void* arg)
{
  while (1)
  {
    WiFiClient client = server->available();
    if (client)
    {
      while (client.connected())
      {
        String line = client.readStringUntil('\r');
        if (line.endsWith("GET /env HTTP/1.1"))
        {
          //Serial.println(recv);
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/plain");
          client.println("Access-Control-Allow-Origin: *");
          client.println();
          client.println("{");
          client.println("\"Temperture\": " +
                         String(tmp, 2) + ",");
          client.println("\"Humidity\": " +
                         String(hum, 2) + ",");
          client.println("\"Pressure\": " +
                         String(pressure, 0));
          client.println("}");
          client.println();
        }
        else
        {
          client.println("HTTP/1.1 404 Not Found");
          client.println();
        }
        client.stop();
      }
    }
    delay(1);
  }
}

void setup()
{
  M5.begin();            // Init M5Stack.
  M5.Power.begin();      // Init power
  if (M5.Power.canControl())
  {
    M5.Power.setPowerVin(false);
  }
  // 320x240
  M5.lcd.setTextSize(3); // Set the text size to 2.
  M5.lcd.setRotation(3);
  Wire.begin();          // Wire init, adding the I2C bus.
  sht30.get();
  qmp6988.init();
  Serial.begin(9600);
  setup_wifi();
  //M5.lcd.println(F("ENV Unit III test"));
  //M5.lcd.sleep();
  xTaskCreatePinnedToCore(fetchSensorData, "Task0", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(processRequest, "Task1", 4096, NULL, 1, NULL, 1);
  setupServer();
}

void loop()
{
  //processRequest();

  displayEnv();
  drawGraph();

  if (!M5.Power.getBatteryLevel() > 90)
  {
    if (!M5.Power.isCharging()) {
      M5.Power.setCharge(true);
    }
    else {
      // do nothing
    }
  }
  else {
    M5.Power.setCharge(false);
  }

  delay(interval);
}