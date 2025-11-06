// ===== smhi_api.ino =====
#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <math.h>
#include <vector>

// Hämtar temperaturer (avrundade °C) kl 12:00Z kommande 7 dagar.
std::vector<int> fetchNoonTemps(double lat, double lon) {
  std::vector<int> temps;

  String url = "https://opendata-download-metfcst.smhi.se/api/category/snow1g/version/1/geotype/point/lon/";
  url += String(lon, 4);
  url += "/lat/";
  url += String(lat, 4);
  url += "/data.json";
  url += "?parameters=air_temperature";

  Serial.print("Hämtar data från: ");
  Serial.println(url);

  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();
  if (httpCode != 200) {
    Serial.printf("HTTP-fel: %d\n", httpCode);
    http.end();
    return temps;  // returnerar tom lista
  }

  String payload = http.getString();
  http.end();

  DynamicJsonDocument doc(250000);
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.printf("JSON error: %s\n", err.c_str());
    return temps;
  }

  JsonArray tsArr = doc["timeSeries"].as<JsonArray>();
  if (tsArr.isNull()) return temps;

  for (JsonObject ts : tsArr) {
    if (temps.size() >= 7) break;

    const char* t = ts["time"] | "";
    if (strstr(t, "T12:00:00Z") == nullptr) continue;

    if (!ts["data"].isNull() && ts["data"].containsKey("air_temperature")) {
      float tempC = ts["data"]["air_temperature"].as<float>();
      temps.push_back((int)roundf(tempC));
    }
  }

  return temps;
}


// Hämtar genomsnittlig daglig temperatur (avrundade °C) för de senaste 3 månaderna.
std::vector<int> fetchPastTemps(int parameter,int station) {
  std::vector<int> temps;

  String url = "https://opendata-download-metobs.smhi.se/api/version/{version}/parameter/";
  url += String(parameter);
  url += "/station/";
  url += String(station);
  url += "/period/{latest-months}/data.json";

  Serial.print("Hämtar data från: ");
  Serial.println(url);

  HTTPClient http;
  http.begin(url);
  http.setTimeout(3000); // Väntar lite så vi inte time-outar API:et
  int httpCode = http.GET();
  if (httpCode != 200) {
    Serial.printf("HTTP-fel: %d\n", httpCode);
    http.end();
    return temps;  // returnerar tom lista
  }

  String payload = http.getString();
  http.end();

  DynamicJsonDocument doc(10000);
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.printf("JSON error: %s\n", err.c_str());
    return temps;
  }

  JsonArray tsArr = doc["value"].as<JsonArray>();
  if (tsArr.isNull()) return temps;

  for (JsonObject ts : tsArr) {
    //if (temps.size() >= 7) break;
    if (!ts.isNull() && ts["data"].containsKey("value")) {
      float tempC = ts["value"].as<float>();
      temps.push_back((int)roundf(tempC));
    }
  }

  return temps;
}
