#include <ArduinoWebsockets.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include "esp_camera.h"

#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

// Motors configuration
#define LEFT_MOTOR 0
#define RIGHT_MOTOR 1

// Pins
static const int LeftMotorPinA = 15;
static const int LeftMotorPinB = 14;

static const int RightMotorPinA = 12;
static const int RightMotorPinB = 13;

// Change this to your network SSID
const char *ssid = "OpenTank";
const char *password = "88888888";

camera_fb_t *fb = NULL;

using namespace websockets;
WebsocketsServer server;
AsyncWebServer webserver(80);

int LeftValue = 0; // Left motor
int RightValue = 0; // Right motor

void setMotorPWM(int motorNumber, int channelValue) {
  int motorFrontPWM, motorBackPWM = 0;

  // Forward
  if (channelValue > 0) {
    motorFrontPWM = map(channelValue, 0, 100, 0, 255); //255 => 8 bit timer
  }
  else {
    motorFrontPWM = 0;
  }

  // Backward
  if (channelValue < 0) {
    motorBackPWM = map(channelValue, -100, 0, 255, 0); //255 => 8 bit timer
  }
  else {
    motorBackPWM = 0;
  }

  if (motorNumber == LEFT_MOTOR) {
    ledcWrite(1, motorFrontPWM);
    ledcWrite(2, motorBackPWM);
  }
  else if (motorNumber == RIGHT_MOTOR) {
    ledcWrite(3, motorFrontPWM);
    ledcWrite(4, motorBackPWM);
  }
}

void setMotors(int steerValue, int forwardValue) {
  int leftMotorValue, rightMotorValue = 0;

  rightMotorValue = forwardValue - steerValue;
  leftMotorValue = forwardValue + steerValue;

  // Constrain values to be between -100 and 100
  rightMotorValue = max(-100, min(100, rightMotorValue));
  leftMotorValue = max(-100, min(100, leftMotorValue));

  if (LeftValue != leftMotorValue) {
    LeftValue = leftMotorValue;
    setMotorPWM(LEFT_MOTOR, LeftValue);
  }
  if (RightValue != rightMotorValue) {
    RightValue = rightMotorValue;
    setMotorPWM(RIGHT_MOTOR, RightValue);
  }
}

void setup()
{
  Serial.begin(115200);

  // Configure motors PWM
  // Forward motor A PWM
  ledcSetup(1, 200, 8);    //channel, freq, resolution
  ledcAttachPin(LeftMotorPinA, 1); // pin, channel

  // Backward motor A PWM
  ledcSetup(2, 200, 8);    //channel, freq, resolution
  ledcAttachPin(LeftMotorPinB, 2); // pin, channel

  // Forward motor B PWM
  ledcSetup(1, 200, 8);    //channel, freq, resolution
  ledcAttachPin(RightMotorPinA, 3); // pin, channel

  // Backward motor B PWM
  ledcSetup(2, 200, 8);    //channel, freq, resolution
  ledcAttachPin(RightMotorPinB, 4); // pin, channel

  for (int i = 0; i < 4; i++) {
    ledcWrite(i, 0);
  }

  // steering servo PWM
  // ledcSetup(4, 50, 16);      //channel, freq, resolution
  // ledcAttachPin(turnPin, 4); // pin, channel

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // Init with high specs to pre-allocate larger buffers
  if (psramFound())
  {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  }
  else
  {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t *s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_SVGA);

  //WiFi create Access point
  WiFi.softAP(ssid, password);

  // This code for AP mode
  // WiFi.mode(WIFI_STA);
  // WiFi.begin(ssid, password);

  Serial.print("AP IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Wait some time to connect to wifi
  for (int i = 0; i < 15 && WiFi.status() != WL_CONNECTED; i++)
  {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP()); // You can get IP address assigned to ESP

  // Initialize SPIFFS
  if (!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  webserver.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Requesting index page...");
    request->send(SPIFFS, "/index.html", "text/html", false);
  });

  // Route to load entireframework.min.css file
  webserver.on("/css/entireframework.min.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/css/entireframework.min.css", "text/css"); });

  // Route to load custom.css file
  webserver.on("/css/custom.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/css/custom.css", "text/css"); });

  // Route to load custom.js file
  webserver.on("/js/custom.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/js/custom.js", "text/javascript"); });

  // Route to load custom.js file
  webserver.on("/js/joy.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/js/joy.js", "text/javascript"); });

  webserver.begin();
  server.listen(82);
  Serial.print("Is server live? ");
  Serial.println(server.available());
}

void handle_message(WebsocketsMessage msg)
{
  int commaIndex = msg.data().indexOf(',');
  int steerValue = msg.data().substring(0, commaIndex).toInt();
  int forwardValue = msg.data().substring(commaIndex + 1).toInt();

  setMotors(steerValue, forwardValue);
}

void loop()
{
  // mjpeg camera stream
  auto client = server.accept();
  client.onMessage(handle_message);
  while (client.available())
  {
    client.poll();
    fb = esp_camera_fb_get();
    client.sendBinary((const char *)fb->buf, fb->len);
    esp_camera_fb_return(fb);
    fb = NULL;
  }
}
