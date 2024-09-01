#include <Arduino.h>
#include <OneWire.h>
#include "DHT.h"
#include <ESP8266WiFi.h>
#include <WebSocketsServer.h>
#include <Hash.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <FS.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>

#define DHTPIN 12
#define DHTTYPE DHT22

WebSocketsServer webSocket = WebSocketsServer(81);
//MDNSResponder mdns;
OneWire ds(13);
DHT dht(DHTPIN, DHTTYPE);

byte data[12];
byte addr[8];
float temperature;
char tempDS18B20[7];
char celsiusTemp[7];
char fahrenheitTemp[7];
char humidityTemp[7];
char Lights[4];
const char* ssid = "wifiloginhere";
const char* password = "wifipasswordhere";
volatile int half_revolutions = 0;
static unsigned long previousMillis;
static unsigned long previousMillis2;
int interval = 3600000;
int interval2 = 5000;
int rpm = 0;
const char* www_username = "admin";
const char* www_password = "password";
const char* host = "ESP8266";
ESP8266WebServer server(80);
byte nbrClient = 0;
unsigned long MymillisCnt = 0;
//String webPage = "";

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) {

  switch (type) {
    case WStype_DISCONNECTED:
      if (nbrClient != 0) nbrClient--;
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        nbrClient++;

      }
      break;
    case WStype_TEXT:
      {

        String text = String((char *) &payload[0]);
        if (text == "wsRefresh") {
          //const char cmdbuffer[] = "<p>testingg</p>";
          FSInfo fs_info;
          SPIFFS.info(fs_info);
          char temp[500];
          int sec = (MymillisCnt + millis()) / 1000;
          int hr = min / 60;
          byte days = hr / 24;
          getDHT();
          if (digitalRead(14)) strcpy (Lights, "Off");
          else strcpy (Lights, "ON");
          long rssi = WiFi.RSSI();
          snprintf (temp, 500, "<p>Uptime: %d Jours %02d:%02d:%02d</p>\
    <p>Temperature: %s C</p>\
    <p>Temperature 2: %s C</p>\
    <p>Humidite: %s %%</p>\
    <p>Force Signal: %d dBm</p>\
    <p>Lumieres: %s </p>\
    <p>Connection: %d</p>\
    <p>Total: %u Bytes</p>\
    <p>Utilise: %u Bytes</p>", days, hr % 24, min % 60, sec % 60, tempDS18B20, celsiusTemp, humidityTemp, rssi, Lights, nbrClient, fs_info.totalBytes, fs_info.usedBytes);
          webSocket.sendTXT(num, temp, strlen(temp));
        }
        else if ( text == "wsDelete" ) {
          SPIFFS.remove("data.log");
          MymillisCnt = 0;
          EEPROM.put(0, MymillisCnt);
        }


      }

      //webSocket.sendTXT(num, payload, lenght);
      //webSocket.broadcastTXT(payload, lenght);
      break;

    case WStype_BIN:

      hexdump(payload, lenght);

      // echo data back to browser
      webSocket.sendBIN(num, payload, lenght);
      break;
  }

}


void handleDownload() {
  if (SPIFFS.exists("data.log")) {
    File file = SPIFFS.open("data.log", "r");
    size_t sent = server.streamFile(file, "application/octet-stream");
    file.close();
  }
  else {
    server.send(404, "text/plain", "404: Not Found");
  }
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for ( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }

  server.send ( 404, "text/plain", message );
}

void handleRoot() {
  if (!server.authenticate(www_username, www_password))
    return server.requestAuthentication();
  char temp[] = "<!DOCTYPE html>\
<html>\
<head>\
<title>ESP8266 Serveur Web</title>\
<style>\
body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
</style>\
<script>\
var connection = new WebSocket('ws://' + location.hostname + ':81/', ['arduino']);\
connection.onopen = function () {\
connection.send('Connect ' + new Date());\
};\
connection.onerror = function (error) {\
console.log('WebSocket Error ', error);\
document.getElementById(\"espdata\").innerHTML = \"<p>ERROR: \" + error.data + \"</p>\";\
};\
connection.onmessage = function (e) {\
console.log('Server: ', e.data);\
document.getElementById(\"espdata\").innerHTML = e.data;\
};\
connection.onclose = function (event) {\
console.log(\"Error occurred.\");\
document.getElementById(\"espdata\").innerHTML = \"<p>Closed: \" + event + \"</p>\";\
};\
var wsCommand = 'wsRefresh';\
function refreshdata() {\
setTimeout('refreshdata()', 5000);\
connection.send(wsCommand);\
}\
function DataDl() {\
document.getElementById('download').click();\
}\
function DataDel() {\
connection.send(\"wsDelete\");\
}\
</script>\
</head>\
<body onload=\"refreshdata()\">\
<h1>Allo depuis ESP8266</h1>\
<div id=\"espdata\"></div>\
<form>\
<input type =\"button\" value=\"Telecharger\" onclick=\"DataDl()\" />\
<input type =\"button\" value=\"Supprimer\" onclick=\"DataDel()\" />\
<a href=\"Data.log\" download id=\"download\" hidden></a>\
</form>\
</body>\
</html>";
  server.send ( 200, "text/html", temp );
}

void LoadConfig() {
  //webPage += "<h1>ESP8266 Web Server</h1><p></p>";
  delay(1000);
  pinMode(14 , INPUT);
  WiFi.hostname(host);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  ArduinoOTA.setPassword((const char *)"225588");
  ArduinoOTA.setHostname(host);
  ArduinoOTA.onEnd([]() {
    EEPROM.put(0, (MymillisCnt + millis()));
    EEPROM.commit();
  });

  ArduinoOTA.begin();
  //mdns.begin("esp8266", WiFi.localIP());
  server.on ( "/", handleRoot );
  server.on ( "/Data.log", handleDownload );
  /*server.on("/", []() {
    getDHT();
    getTemp(&temperature);
    long rssi = WiFi.RSSI();
    webPage = "<h1>ESP8266 Server Web</h1>";
    webPage += "<p>Temperature: " + String(temperature) + " C </p>";
    webPage += "<p>Temperature2: " + String(celsiusTemp) + " C </p>";
    webPage += "<p>TemperatureF: " + String(fahrenheitTemp) + " F </p>";
    webPage += "<p>Humidite: " + String(humidityTemp) + " % </p>";
    webPage += "<p>Force Signal: " + String(rssi) + " </p>";
    server.send(200, "text/html", webPage);
    });*/
  server.onNotFound ( handleNotFound );
  server.begin();
  dht.begin();
  if ( !ds.search(addr)) {
    ds.reset_search();
  }
  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);
  //attachInterrupt(14, rpm_fan, FALLING);
  SPIFFS.begin();
  //Serial.begin(9600);
}

void rpm_fan() {
  half_revolutions++;
}


void setup() {
  // put your setup code here, to run once:
  EEPROM.begin(512);
  EEPROM.get(0, MymillisCnt);
  LoadConfig();
}


void loop() {
  // put your main code here, to run repeatedly:

  webSocket.loop();
  server.handleClient();
  ArduinoOTA.handle();
  if ((long)(millis() - previousMillis) >= 0) {
    previousMillis += interval;
    char filename [] = "data.log";
    File myDataFile = SPIFFS.open(filename, "a+");
    if (digitalRead(14)) myDataFile.print("0,");
    else myDataFile.print("1,");
    getDHT();
    myDataFile.print(tempDS18B20);
    myDataFile.print(",");
    myDataFile.println(humidityTemp);
    myDataFile.close();
    //rpm = half_revolutions * 6;
    //half_revolutions = 0;
  }
    if ((long)(millis() - previousMillis2) >= 0) {
    previousMillis2 += interval2;
    getTemp(&temperature);
    dtostrf(temperature, 5, 2, tempDS18B20);
    /*Serial.print('<');
    Serial.print(tempDS18B20);
    Serial.print('>');*/
  }

}


void getTemp(float *dsTemp) {
  //Serial.println("get temp");

  ds.reset();
  ds.select(addr);
  ds.write(0xBE);


  for (int i = 0; i < 9; i++) {
    data[i] = ds.read();
  }

  byte LSB = data[0];
  byte MSB = data[1];
  int tempRead = ((MSB << 8) | LSB);
  float TemperatureSum = tempRead * 0.0625;


  byte present = ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);
  *dsTemp = TemperatureSum;

}

void getDHT() {
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  float hic = dht.computeHeatIndex(t, h, false);
  dtostrf(hic, 6, 2, celsiusTemp);
  float hif = dht.computeHeatIndex(f, h);
  dtostrf(hif, 6, 2, fahrenheitTemp);
  dtostrf(h, 3, 0, humidityTemp);
}
