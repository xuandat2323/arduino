#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>

// ==== CẤU HÌNH WIFI ====
const char* ssid = "FPT Telecom";
const char* password = "Hadat2364";

// ==== ĐỊA CHỈ SERVER FLASK ====
const char* flaskServer = "http://192.168.1.173:5000/upload"; // Sửa IP nếu cần

// ==== CHÂN CAMERA (AI Thinker) ====
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// ==== CHÂN PHÁT HIỆN CHUYỂN ĐỘNG ====
#define MOTION_PIN 13

void setupCamera() {
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

  if(psramFound()) {
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_CIF;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera init failed");
    while (true);
  }
}

void sendToFlaskServer(camera_fb_t *fb) {
  if (!fb) {
    Serial.println("Capture failed!");
    return;
  }

  WiFiClient client;
  HTTPClient http;

  Serial.println("[HTTP] Sending image to Flask...");
  http.begin(client, flaskServer);
  http.addHeader("Content-Type", "image/jpeg");

  int httpResponseCode = http.sendRequest("POST", fb->buf, fb->len);

  if (httpResponseCode > 0) {
    Serial.printf("Flask response: %d\n", httpResponseCode);
    String response = http.getString();
    Serial.println("Response: " + response);
  } else {
    Serial.printf("POST failed: %s\n", http.errorToString(httpResponseCode).c_str());
  }

  http.end();
  esp_camera_fb_return(fb);
}

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32-CAM with motion detection");

  // Kết nối WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected! IP:");
  Serial.println(WiFi.localIP());

  // Camera & PIR
  setupCamera();
  pinMode(MOTION_PIN, INPUT);
}

void loop() {
  if (digitalRead(MOTION_PIN) == HIGH) {
    Serial.println("📷 Motion detected!");

    camera_fb_t *fb = esp_camera_fb_get();
    sendToFlaskServer(fb);
    esp_camera_fb_return(fb);

    delay(5000); // Tránh chụp quá nhiều liên tiếp
  }

  delay(100); // kiểm tra mỗi 100ms
}
