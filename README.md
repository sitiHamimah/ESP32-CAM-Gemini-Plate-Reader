# 🚗 ESP32-CAM AI Plate Recognizer
An IoT project that captures vehicle images using an ESP32-CAM, uses **Google Gemini 2.0 Flash AI** to read the license plate, and logs the data to **Firebase Realtime Database**.

## 🛠️ Features
- **AI OCR:** Uses Gemini 2.0 Flash for high-accuracy text recognition.
- **Real-time Database:** Logs plate numbers and timestamps to Firebase.
- **Web Dashboard:** A simple HTML/JS interface to view detections instantly.

## 📂 Project Structure
- `/arduino`: Contains the `.ino` sketch for ESP32.
- `/web`: Contains the `index.html` dashboard.

## 🚀 Setup Instructions
1. **Gemini API:** Get a key from Google AI Studio.
2. **Firebase:** Create a Realtime Database and set rules to public for testing.
3. **Hardware:** Use an ESP32-CAM (Ai-Thinker).
4. **Library Requirements:** ArduinoJson, ESP32cam.

## 📸 Demo
(Optional: Upload a screenshot of your Serial Monitor or Web Dashboard here!)
