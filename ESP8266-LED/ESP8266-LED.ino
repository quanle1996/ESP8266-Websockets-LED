/*
  Скетч разработан 30.11.2018 Wirekraken
*/
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <FS.h>

#define FASTLED_ESP8266_NODEMCU_PIN_ORDER
#define ESP8266_SPI
#include <cstdlib>
#include <SPI.h>
#include <FastLED.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <string>

const char *ssid = "Regenesis Studio"; // name of your network
const char *password = "Pod_00001";    // your network password
const uint16_t mqtt_port = 1883;
const char *mqtt_username = "mod_tabui";
const char *mqtt_password = "mod_tabui";
const std::string mqtt_prefix = "pod/0000/1.0/mod_lighting/";

const std::string mqtt_led_event_level = (mqtt_prefix + "rgb/level/set");
const char *mqtt_led_level = (mqtt_prefix + "rgb/level/set").c_str();
const std::string mqtt_led_event_main = (mqtt_prefix + "rgb/main");
const char *mqtt_led_main = (mqtt_prefix + "rgb/level/set").c_str();

IPAddress Mqtt_server(192, 168, 50, 50);
IPAddress Ip(192, 168, 50, 24);     // IP address for ESP
IPAddress Gateway(192, 168, 50, 1); // IP address of the gateway (router)
IPAddress Subnet(255, 255, 255, 0); // subnet mask, range of IP addresses in the local network

#define LED_COUNT 60 // number of led in the tape
#define LED_DT 2     // pin where is connected the DIN strips (number of pins coincides with the ESP8266 Arduino)
#define LEDS FastLED

uint8_t bright = 50; // bright (0 - 255)
uint8_t ledMode = 0; // trigger effect (0 - 29)

uint8_t flag = 1; // effect undo flag

CRGBArray<LED_COUNT> leds;

uint8_t delayValue = 20; // delay
uint8_t stepValue = 10;  // pixel pitch
uint8_t hueValue = 0;    // color tone

// инициализация websocket на 81 порту
WebSocketsServer webSocket(81);
ESP8266WebServer server(80);

WiFiClient espClient;
PubSubClient client(Mqtt_server, mqtt_port, espClient);
unsigned long lastMsg = 0;
long lastReconnectAttempt = 0;

#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

void setup_wifi()
{
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.config(Ip, Gateway, Subnet);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  client.setServer(Mqtt_server, mqtt_port);
  client.setCallback(callback);

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  if (mqtt_led_event_main.compare(topic) == 0 && length == 7)
  {
    uint8_t chars[length + 1];
    memcpy(chars, payload, length);
    // Serial.printf((char *)(uint8_t *)payload);
    chars[length] = '\0';
    setLEDColor(chars);
  }
  else if (mqtt_led_event_level.compare(topic) == 0 && length <= 3)
  {
    String data;
    for (int x = 0; x < length; x++)
    {
      if (!isdigit(payload[x]))
        continue;
      data += (char)payload[x];
    }
    Serial.print("Bright: ");
    bright = data.toInt();
    Serial.println(data);
    LEDS.setBrightness(bright);
  }
}

boolean reconnect()
{
  // Create a random client ID
  String clientId = "ESP8266Client-";
  clientId += String(random(0xffff), HEX);
  // Attempt to connect
  if (client.connect(clientId.c_str(), mqtt_username, mqtt_password))
  {
    // ... and resubscribe
    client.subscribe(mqtt_led_main);
    client.subscribe(mqtt_led_level);
  }
  return client.connected();
}

void setup()
{
  Serial.begin(9600);
  LEDS.setBrightness(bright);

  LEDS.addLeds<WS2811, LED_DT, GRB>(leds, LED_COUNT); // настройки для вашей ленты (ленты на WS2811, WS2812, WS2812B)
  updateColor(0, 0, 0);
  LEDS.show();

  setup_wifi();

  server.onNotFound([]()
                    {
    if(!handleFileRead(server.uri()))
      server.send(404, "text/plain", "FileNotFound"); });
  server.begin();
  SPIFFS.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop()
{
  //обработка входящих запросов HTTP или WebSockets
  webSocket.loop();
  server.handleClient();
  ledEffect(ledMode);
  long now = millis();
  if (!client.connected())
  {
    if (now - lastReconnectAttempt > 5000)
    {
      lastReconnectAttempt = now;
      if (reconnect())
      {
        lastReconnectAttempt = 0;
      }
    }
  }
  else
  {
    client.loop();
  }
  if (now - lastMsg > 5000)
  {
    lastMsg = now;
    ++value;
    snprintf(msg, MSG_BUFFER_SIZE, "Led arduino Online #%ld", value);
    // Serial.print("Publish message: ");
    // Serial.println();
    if (client.connected())
    {
      client.publish("outTopic", msg);
    }
  }
}

void setLEDColor(uint8_t *payload)
{
  // convert to 24 bit color number
  uint32_t rgb = (uint32_t)strtol((const char *)&payload[1], NULL, 16);

  // convert 24 bits to 8 bits per channel
  uint8_t r = abs(int(0 + (rgb >> 16) & 0xFF));
  uint8_t g = abs(int(0 + (rgb >> 8) & 0xFF));
  uint8_t b = abs(int(0 + (rgb >> 0) & 0xFF));

  Serial.println((char *)payload);
  Serial.print("ColorPicker: ");
  Serial.print(r);
  Serial.print(g);
  Serial.println(b);

  for (int x = 0; x < LED_COUNT; x++)
  {
    leds[x].setRGB(r, g, b);
  }
  LEDS.show();
}

// function for processing incoming messages
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{

  if (type == WStype_CONNECTED)
  {
    IPAddress ip = webSocket.remoteIP(num);

    String message = String("Connected");
    webSocket.broadcastTXT(message); // send the last value to all clients on connection
  }

  if (type == WStype_TEXT)
  {
    String data;
    for (int x = 0; x < length; x++)
    {
      if (!isdigit(payload[x]))
        continue;
      data += (char)payload[x];
    }

    if (payload[0] == 'B')
    {
      flag = 0;
      Serial.print("Bright: ");
      bright = data.toInt();
      Serial.println(data);
      LEDS.setBrightness(bright);
    }
    else if (payload[0] == 'F')
    {
      flag = 0;
      Serial.print("Function: ");
      ledMode = data.toInt();
      Serial.println(data);
      ledEffect(ledMode);
    }
    else if (payload[0] == '#')
    {

      if (!flag)
      {
        Serial.print("flag : ");
        Serial.println(flag);
        ledMode = flag;
        ledEffect(ledMode);
        flag = 1;
      }
      else
      {
        setLEDColor(payload);
      }
    }
  }
}

// функция эффектов
void ledEffect(int ledMode)
{
  switch (ledMode)
  {
  case 0:
    updateColor(0, 0, 0);
    break;
  case 1:
    rainbow_fade();
    delayValue = 20;
    break;
  case 2:
    rainbow_loop();
    delayValue = 20;
    break;
  case 3:
    new_rainbow_loop();
    delayValue = 5;
    break;
  case 4:
    random_march();
    delayValue = 40;
    break;
  case 5:
    rgb_propeller();
    delayValue = 25;
    break;
  case 6:
    rotatingRedBlue();
    delayValue = 40;
    hueValue = 0;
    break;
  case 7:
    Fire(55, 120, delayValue);
    delayValue = 15;
    break;
  case 8:
    blueFire(55, 250, delayValue);
    delayValue = 15;
    break;
  case 9:
    random_burst();
    delayValue = 20;
    break;
  case 10:
    flicker();
    delayValue = 20;
    break;
  case 11:
    random_color_pop();
    delayValue = 35;
    break;
  case 12:
    Sparkle(255, 255, 255, delayValue);
    delayValue = 0;
    break;
  case 13:
    color_bounce();
    delayValue = 20;
    hueValue = 0;
    break;
  case 14:
    color_bounceFADE();
    delayValue = 40;
    hueValue = 0;
    break;
  case 15:
    red_blue_bounce();
    delayValue = 40;
    hueValue = 0;
    break;
  case 16:
    rainbow_vertical();
    delayValue = 50;
    stepValue = 15;
    break;
  case 17:
    matrix();
    delayValue = 50;
    hueValue = 95;
    break;

  // тяжелые эффекты
  case 18:
    rwb_march();
    delayValue = 80;
    break;
  case 19:
    flame();
    break;
  case 20:
    theaterChase(255, 0, 0, delayValue);
    delayValue = 50;
    break;
  case 21:
    Strobe(255, 255, 255, 10, delayValue, 1000);
    delayValue = 100;
    break;
  case 22:
    policeBlinker();
    delayValue = 25;
    break;
  case 23:
    kitt();
    delayValue = 100;
    break;
  case 24:
    rule30();
    delayValue = 100;
    break;
  case 25:
    fade_vertical();
    delayValue = 60;
    hueValue = 180;
    break;
  case 26:
    fadeToCenter();
    break;
  case 27:
    runnerChameleon();
    break;
  case 28:
    blende();
    break;
  case 29:
    blende_2();
  }
}

// функция получения типа файла
String getContentType(String filename)
{
  if (server.hasArg("download"))
    return "application/octet-stream";
  else if (filename.endsWith(".htm"))
    return "text/html";
  else if (filename.endsWith(".html"))
    return "text/html";
  else if (filename.endsWith(".css"))
    return "text/css";
  else if (filename.endsWith(".js"))
    return "application/javascript";
  else if (filename.endsWith(".png"))
    return "image/png";
  else if (filename.endsWith(".gif"))
    return "image/gif";
  else if (filename.endsWith(".jpg"))
    return "image/jpeg";
  else if (filename.endsWith(".ico"))
    return "image/x-icon";
  else if (filename.endsWith(".xml"))
    return "text/xml";
  else if (filename.endsWith(".pdf"))
    return "application/x-pdf";
  else if (filename.endsWith(".zip"))
    return "application/x-zip";
  else if (filename.endsWith(".gz"))
    return "application/x-gzip";
  return "text/plain";
}

// функция поиска файла в файловой системе
bool handleFileRead(String path)
{
#ifdef DEBUG
  Serial.println("handleFileRead: " + path);
#endif
  if (path.endsWith("/"))
    path += "index.html";
  if (SPIFFS.exists(path))
  {
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, getContentType(path));
    file.close();
    return true;
  }
  return false;
}
