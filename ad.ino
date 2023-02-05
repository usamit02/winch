
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <FS.h>
#include <Ticker.h>
const int pin1 = 3;
const int pin2 = 0;
int state      = 0;
int keep       = 0;
int wait       = 0;
struct Config
{
  uint8_t ip[4] = {192, 168, 4, 2};
  char ssid[16] = "ESP", pass[16];
  int sec       = 1;
  int keep      = 3;
};
IPAddress subnet(255, 255, 255, 0);
ESP8266WebServer server(80);
Ticker ticker;
Config conf;

void handleRoot() {
  String s;
  if (server.hasArg("req")) {
    if (server.arg("req") == "Fow->" && state != -1 && wait == 0) {
      digitalWrite(pin1, 1);
      state = 1;
      keep  = conf.keep;
      Serial.println("state = 1");
    }
    if (server.arg("req") == "<-Rew" && state != 1 && wait == 0) {
      digitalWrite(pin2, 1);
      state = -1;
      keep  = conf.keep;
      Serial.println("state = -1");
    }
    if (server.arg("req") == "Stop") {
      digitalWrite(pin1, 0);
      digitalWrite(pin2, 0);
      state = 0;
      wait  = 2;
      Serial.println("state = 0");
    }
  }
  s = "<html><head><meta http-equiv='Content-Type' content='text/html; charset=UTF-8'></head>";
  s += "<body><form name='f1' method='get'>";
  s += "<input type='submit' name='req' value='<-Rew' style='font-size:10em;color:yellow;width:100%;margin-top:0.5em;'>";
  s += "<input type='submit' name='req' value='Fow->' style='font-size:10em;color:blue;width:100%;margin-top:1.5em;'>";
  s += "<input type='submit' name='req' value='Stop' style='font-size:10em;color:red;width:100%;margin-top:1.5em;'></form>";
  s += "<button type='button' onclick='location.href=\"conf\"' style='font-size:5em;width:100%;margin-top:3em;'>config</button></body></html>";
  server.send(200, "text/html", s);
}

void handleConf() {
  if (server.hasArg("conf")) {
    if (server.arg("conf") == "update") {
      for (int i = 0; i < 4; i++) {
        conf.ip[i] = server.arg("ip" + String(i)).toInt();
      }
      server.arg("ssid").toCharArray(conf.ssid, 16);
      server.arg("pass").toCharArray(conf.pass, 16);
      conf.sec  = server.arg("sec").toInt();
      conf.keep = server.arg("keep").toInt();
      EEPROM.write(0, 100);
    } else {
      Config c;
      conf = c;
    }
    EEPROM.put<Config>(1, conf);
    EEPROM.commit();
    handleRoot();
  } else {
    String s;
    s = "<html><head><meta http-equiv='Content-Type' content='text/html; charset=UTF-8'></head>";
    s += "<body><h1>Configlation</h1><form method='get' action='conf'>IPaddress";
    for (int i = 0; i < 4; i++) {
      s += "<input type = 'number' name = 'ip" + String(i) + "' value = '" + String(conf.ip[i]) + "' size='3' min='0' max='255' required >.";
    }
    s += "<br>SSID<input type = 'text' name = 'ssid' value = '" + String(conf.ssid) + "' required >";
    s += "<br>password<input type = 'password' name = 'pass' value = '" + String(conf.pass) + "'>";
    s += "<br>sec<input type='number' name='sec' value='" + String(conf.sec) + "' required >";
    s += "<br>keep<input type='number' name='keep' value='" + String(conf.keep) + "' required >";
    s += "<br><input type='submit' name='conf' value='update'><input type='submit' name='conf' value='default'></form></body></html>";
    server.send(200, "text/html", s);
  }
}

void setup() {
  EEPROM.begin(1000);
  SPIFFS.begin();
  File f = SPIFFS.open("/temp1.dat", "w");
  f.close();
  Serial.begin(115200);
  Serial.println();
  Serial.print("Configuring access point...");
  if (EEPROM.read(0) == 100) {
    EEPROM.get<Config>(1, conf);
  }
  IPAddress ip = conf.ip;
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(ip, ip, subnet);
  WiFi.softAP(conf.ssid);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.on("/", handleRoot);
  server.on("/conf", handleConf);
  server.begin();
  Serial.println("HTTP server started");
  ticker.attach(conf.sec, tick);
  pinMode(pin1, OUTPUT);
  digitalWrite(pin1, 0);
  pinMode(pin2, OUTPUT);
  digitalWrite(pin2, 0);
  digitalWrite(1, 0);
}

void tick() {
  Serial.print("ticker,");
  if (wait > 0) {
    wait--;
    Serial.println("wait...");
  }
  if (state != 0) {
    keep--;
    if (keep < 1) {
      digitalWrite(pin1, 0);
      digitalWrite(pin2, 0);
      state = 0;
      Serial.println("keep out");
    } else {
      Serial.println("keep...");
    }
  }
}

void loop() {
  server.handleClient();
}
