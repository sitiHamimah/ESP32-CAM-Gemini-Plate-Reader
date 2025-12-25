/*
 * Project: AI Plate Recognition (ANPR)
 * Hardware: ESP32-CAM (Ai-Thinker)
 * AI: Google Gemini 2.0 Flash
 * Database: Firebase Realtime Database
 */

#include <esp32cam.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include "esp_camera.h"
#include "time.h"

// ================= WIFI & API CONFIG =================
const char* WIFI_SSID = "Your_SSID";
const char* WIFI_PASS = "Your_Password";
const char* GEMINI_API_KEY = "YOUR_API_KEY_HERE";
const char* FIREBASE_URL = "https://your-project.firebaseio.com/numberplate.json";

const char* ntpServer = "pool.org";
// =====================================================

String lastDetectedPlate = ""; // Variable to prevent duplicate logs

// Base64 Encoding Function
const char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
String base64_encode(const uint8_t* data, size_t length) {
    String encoded = "";
    int i = 0;
    uint8_t array_3[3], array_4[4];
    while (length--) {
        array_3[i++] = *(data++);
        if (i == 3) {
            array_4[0] = (array_3[0] & 0xfc) >> 2;
            array_4[1] = ((array_3[0] & 0x03) << 4) + ((array_3[1] & 0xf0) >> 4);
            array_4[2] = ((array_3[1] & 0x0f) << 2) + ((array_3[2] & 0xc0) >> 6);
            array_4[3] = array_3[2] & 0x3f;
            for (i = 0; i < 4; i++) encoded += base64_table[array_4[i]];
            i = 0;
        }
    }
    if (i) {
        for (int j = i; j < 3; j++) array_3[j] = '\0';
        array_4[0] = (array_3[0] & 0xfc) >> 2;
        array_4[1] = ((array_3[0] & 0x03) << 4) + ((array_3[1] & 0xf0) >> 4);
        array_4[2] = ((array_3[1] & 0x0f) << 2) + ((array_3[2] & 0xc0) >> 6);
        for (int j = 0; j < i + 1; j++) encoded += base64_table[array_4[j]];
        while (i++ < 3) encoded += '=';
    }
    return encoded;
}

String getCurrentTime() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) return "Time Error";
    char buffer[30];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return String(buffer);
}

void sendDataToFirebase(String numberPlate, String dateTime) {
    WiFiClientSecure *f_client = new WiFiClientSecure;
    if(f_client) {
        f_client->setInsecure();
        HTTPClient http;
        
        http.begin(*f_client, FIREBASE_URL); 
        http.addHeader("Content-Type", "application/json");
        
        String payload = "{\"number_plate\":\"" + numberPlate + "\",\"date_time\":\"" + dateTime + "\"}";
        
        int httpCode = http.POST(payload);
        if (httpCode > 0) {
            Serial.println("[+] Firebase Updated: " + http.getString());
        } else {
            Serial.printf("[-] Firebase Error: %d\n", httpCode);
        }
        http.end();
        delete f_client;
    }
}

void detectNumberPlate() {
    Serial.println("\n[+] Capturing Image...");
    auto frame = esp32cam::capture();
    if (frame == nullptr) {
        Serial.println("[-] Capture failed");
        return;
    }
    
    String base64Image = base64_encode(frame->data(), frame->size());
    
    WiFiClientSecure *client = new WiFiClientSecure;
    if(client) {
        client->setInsecure(); 
        HTTPClient http;
        String url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.0-flash:generateContent?key=" + String(GEMINI_API_KEY);
        
        if (http.begin(*client, url)) { 
            http.setTimeout(25000); // 25s timeout for stability
            http.addHeader("Content-Type", "application/json");
            
            String payload = "{\"contents\":[{\"parts\":["
                             "{\"text\":\"Return ONLY the vehicle license plate text. If none, return 'No Plate'.\"},"
                             "{\"inline_data\":{\"mime_type\":\"image/jpeg\",\"data\":\"" + base64Image + "\"}}"
                             "]}]}";
            
            int httpCode = http.POST(payload);
            
            if (httpCode == 200) {
                String response = http.getString();
                DynamicJsonDocument doc(4096);
                deserializeJson(doc, response);
                
                const char* extractedText = doc["candidates"][0]["content"]["parts"][0]["text"];
                String plate = String(extractedText);
                plate.trim();

                if (plate != "" && plate != "No Plate" && plate.length() > 2) {
                    // Check for duplicate before sending
                    if (plate != lastDetectedPlate) {
                        Serial.println("\n[!] NEW PLATE DETECTED: " + plate);
                        sendDataToFirebase(plate, getCurrentTime());
                        lastDetectedPlate = plate; // Update last detected
                    } else {
                        Serial.println("[.] Same plate detected, skipping Firebase...");
                    }
                } else {
                    Serial.println("[.] No plate in frame.");
                    lastDetectedPlate = ""; // Clear so it can detect the next car
                }
            } else {
                Serial.printf("[-] Gemini Error: %d - %s\n", httpCode, http.errorToString(httpCode).c_str());
            }
            http.end();
        }
        delete client;
    }
}

void setup() {
    Serial.begin(115200);
    
    // Connect WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\n[+] WiFi Connected");

    // Camera Config
    using namespace esp32cam;
    Config cfg;
    cfg.setPins(pins::AiThinker);
    cfg.setResolution(Resolution::find(640, 480)); 
    cfg.setJpeg(80);
    
    if (!Camera.begin(cfg)) {
        Serial.println("[-] Camera Initialization Failed");
        ESP.restart();
    }
    
    // Sync Time
    configTime(0, 0, ntpServer);
    Serial.println("[+] System Ready");
}

void loop() {
    detectNumberPlate();
    delay(8000); // Wait 8 seconds between captures
}