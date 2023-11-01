#include <ArduinoWebsockets.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include "esp_camera.h"

// Access point WiFi mode, comment it if you need to wifi client mode
#define AP

// Motors configuration
static const uint8_t LEFT_MOTOR = 0;
static const uint8_t RIGHT_MOTOR = 1;

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
WebsocketsServer websocketserver;
AsyncWebServer webserver(80);

int LeftValue = 0; // Left motor
int RightValue = 0; // Right motor

void setMotorPWM(int motorNumber, int channelValue) {
  int motorFrontPWM = 0;
  int motorBackPWM = 0;

  if (channelValue > 0) { // Forward 0..100
    motorFrontPWM = map(channelValue, 0, 100, 0, 255); //255 => 8 bit timer
  } else
  
  if (channelValue < 0) { // Backward
    motorBackPWM = map(channelValue, -100, 0, 255, 0); //255 => 8 bit timer
  } 

  if (motorNumber == LEFT_MOTOR) {
    ledcWrite(8, motorFrontPWM);
    ledcWrite(2, motorBackPWM);
  }
  else if (motorNumber == RIGHT_MOTOR) {
    ledcWrite(3, motorFrontPWM);
    ledcWrite(4, motorBackPWM);
  }
}

void setMotors(int steerValue, int forwardValue) {
  // Calculate the PWM morotors values
  int rightMotorValue = forwardValue - steerValue;
  int leftMotorValue = forwardValue + steerValue;

  // Constrain values to be between -100 and 100
  rightMotorValue = max(-100, min(100, rightMotorValue));
  leftMotorValue = max(-100, min(100, leftMotorValue));

  // 2
  if (LeftValue != leftMotorValue) {
    LeftValue = leftMotorValue;
    setMotorPWM(LEFT_MOTOR, LeftValue);
  }

  if (RightValue != rightMotorValue) {
    RightValue = rightMotorValue;
    setMotorPWM(RIGHT_MOTOR, RightValue);
  }
}

void configureMotors() {
  // Forward motor A (Left) PWM
  // 1st channel does not work correctly in this configuration!
  ledcSetup(8, 200, 8);    //channel, freq, resolution
  ledcAttachPin(LeftMotorPinA, 8); // pin, channel

  // Backward motor A (Left) PWM
  ledcSetup(2, 200, 8);    //channel, freq, resolution
  ledcAttachPin(LeftMotorPinB, 2); // pin, channel

  // Forward motor B (Right) PWM
  ledcSetup(3, 200, 8);    //channel, freq, resolution
  ledcAttachPin(RightMotorPinA, 3); // pin, channel

  // Backward motor B (Right) PWM
  ledcSetup(4, 200, 8);    //channel, freq, resolution
  ledcAttachPin(RightMotorPinB, 4); // pin, channel
}

void configureWiFi() {
  // WiFi Access point
  #ifdef AP
    WiFi.softAP(ssid, password);
    Serial.print("AP IP Address: ");
    Serial.println(WiFi.softAPIP());
  #endif

  // WiFi station
  #ifndef AP
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    // Waiting for connection to wifi
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      delay(1000);
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  #endif
}

void configureCamera() {
  // Camera module pinout configuration
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = 5; // Y2_GPIO_NUM
  config.pin_d1 = 18; // Y3_GPIO_NUM
  config.pin_d2 = 19; // Y4_GPIO_NUM
  config.pin_d3 = 21; // Y5_GPIO_NUM
  config.pin_d4 = 36; // Y6_GPIO_NUM
  config.pin_d5 = 39; // Y7_GPIO_NUM
  config.pin_d6 = 34; // Y8_GPIO_NUM
  config.pin_d7 = 35; // Y9_GPIO_NUM
  config.pin_xclk = 0; // XCLK_GPIO_NUM
  config.pin_pclk = 22; // PCLK_GPIO_NUM
  config.pin_vsync = 25; // VSYNC_GPIO_NUM
  config.pin_href = 23; // HREF_GPIO_NUM
  config.pin_sscb_sda = 26; // SIOD_GPIO_NUM
  config.pin_sscb_scl = 27; // SIOC_GPIO_NUM
  config.pin_pwdn = 32; // PWDN_GPIO_NUM
  config.pin_reset = -1; // RESET_GPIO_NUM
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // Init with high specs to pre-allocate larger buffers
  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t *s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_SVGA);
}

void configureWebSerwer() {
  // Route default path
  webserver.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Requesting index page...");
    request->send(SPIFFS, "/index.html", "text/html", false);
  });

  // Route to load index.css file
  webserver.on("/css/index.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/css/index.css", "text/css"); });

  // Route to load index.js file
  webserver.on("/js/index.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/js/index.js", "text/javascript"); });

  // Route to load joy.js file
  webserver.on("/js/joy.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/js/joy.js", "text/javascript"); });

  webserver.begin();
  websocketserver.listen(82);
  Serial.print("Is server live? ");
  Serial.println(websocketserver.available());
}

void setup() {
  Serial.begin(115200);
  Serial.println("Setup");

  configureMotors();

  configureCamera();

  configureWiFi();

  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Configure webserver
  configureWebSerwer();
}

void handle_message(WebsocketsMessage msg) {
  int commaIndex = msg.data().indexOf(',');
  int steerValue = msg.data().substring(0, commaIndex).toInt();
  int forwardValue = msg.data().substring(commaIndex + 1).toInt();

  Serial.println(forwardValue);
  setMotors(steerValue, forwardValue);
}

int counter = 0;
int digit = 1;

void loop() {
  // Mjpeg camera stream
  auto client = websocketserver.accept();
  client.onMessage(handle_message);

  while (client.available()) {
    client.poll();
    fb = esp_camera_fb_get();
    client.sendBinary((const char *)fb->buf, fb->len);
    esp_camera_fb_return(fb);
    fb = NULL;
  }
}
