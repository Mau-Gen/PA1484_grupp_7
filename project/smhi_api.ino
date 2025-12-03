// ===== smhi_api.ino =====
#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <math.h>
#include <vector>

// Hämtar data kl 12:00Z kommande 7 dagar.
std::vector<int> fetchNoonData(String str, double lat, double lon) {
  std::vector<int> data;

  String url = "https://opendata-download-metfcst.smhi.se/api/category/snow1g/version/1/geotype/point/lon/";
  url += String(lon, 4);
  url += "/lat/";
  url += String(lat, 4);
  url += "/data.json";
  url += "?parameters=";
  url += str;

  Serial.print("Hämtar data från: ");
  Serial.println(url);

  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();
  if (httpCode != 200) {
    Serial.printf("HTTP-fel: %d\n", httpCode);
    http.end();
    return data;  // returnerar tom lista
  }

  String payload = http.getString();
  http.end();

  DynamicJsonDocument doc(250000);
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.printf("JSON error: %s\n", err.c_str());
    return data;
  }

  JsonArray tsArr = doc["timeSeries"].as<JsonArray>();
  if (tsArr.isNull()) return data;

  for (JsonObject ts : tsArr) {
    if (data.size() >= 7) break;

    const char* t = ts["time"] | "";
    if (strstr(t, "T12:00:00Z") == nullptr) continue;

    if (!ts["data"].isNull() && ts["data"].containsKey(str)) {
      float value = ts["data"][str].as<float>();
      data.push_back((int)roundf(value));
    }
  }

  return data;
}


// Hämtar genomsnittlig daglig temperatur (avrundade °C) för de senaste 3 månaderna.
std::vector<int> fetchPastData(int parameter,int station) {
  std::vector<int> data;

  String url = "https://opendata-download-metobs.smhi.se/api/version/latest/parameter/";
  url += String(parameter);
  url += "/station/";
  url += String(station);
  url += "/period/latest-months/data.json";

  Serial.print("Hämtar data från: ");
  Serial.println(url);

  HTTPClient http;
  http.begin(url);
  http.setTimeout(3000); // Väntar lite så vi inte time-outar API:et
  int httpCode = http.GET();
  if (httpCode != 200) {
    Serial.printf("HTTP-fel: %d\n", httpCode);
    http.end();
    return data;  // returnerar tom lista
  }

  String payload = http.getString();
  http.end();

  //StaticJsonDocument<200> filter;
  //filter["value"][0]["value"] = true;
  DynamicJsonDocument doc(15000);
  DeserializationError err = deserializeJson(doc,payload);


  if (err) {
    Serial.printf("JSON error: %s\n", err.c_str());
    return data;
    
  }

  JsonArray tsArr = doc["value"].as<JsonArray>();
  if (tsArr.isNull()) {
    Serial.println("API:et Är tomt");
    return data;
    
  
  };

  for (JsonObject ts : tsArr) {
    if (!ts.isNull()) {
      int tempC = ts["value"].as<int>();
      data.push_back((tempC));
    }
  }

  return data;
}
