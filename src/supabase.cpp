#include "config.h"
#include "supabase.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

bool fetchSupabaseData() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Supabase fetch skipped: WiFi not connected");
    return false;
  }

  // TLS 1.2 encrypted transport.
  // NOTE: On ESP32 without Secure Boot + Flash Encryption, the compiled key
  // can be extracted from flash. Enable Flash Encryption + Secure Boot in efuse
  // for true at-rest protection.
  WiFiClientSecure secureClient;
  secureClient.setInsecure(); // Production: pin CA cert

  HTTPClient http;
  http.setTimeout(10000);
  http.begin(secureClient, supabaseUrl);
  http.addHeader("apikey", supabaseKey);
  http.addHeader("Authorization", String("Bearer ") + supabaseKey);
  http.addHeader("Accept", "application/json");

  int httpCode = http.GET();

  if (httpCode != 200) {
    Serial.printf("Supabase fetch failed: HTTP %d\r\n", httpCode);
    http.end();
    return false;
  }

  String response = http.getString();
  http.end();

#if ARDUINOJSON_VERSION_MAJOR >= 7
  JsonDocument doc;
#else
  DynamicJsonDocument doc(8192);
#endif

  DeserializationError error = deserializeJson(doc, response);
  if (error) {
    Serial.printf("Supabase JSON parse failed: %s\r\n", error.c_str());
    return false;
  }

  JsonArray employees = doc.as<JsonArray>();
  int count = 0;
  for (JsonObject emp : employees) {
    if (count >= maxEmployees) break;

    String firstName = emp["first_name"] | "";
    String lastName  = emp["last_name"]  | "";
    employeeData[count][0] = firstName + " " + lastName;
    employeeData[count][1] = emp["position"] | "";

    const char* startStr = emp["schedule_start"];
    const char* endStr   = emp["schedule_end"];
    if (startStr && endStr) {
      String startTime = String(startStr);
      String endTime   = String(endStr);
      if (startTime.length() >= 16 && endTime.length() >= 16) {
        employeeData[count][2] = startTime.substring(11, 16) + " - " + endTime.substring(11, 16);
        employeeScheduleDay[count] = (uint8_t)startTime.substring(8, 10).toInt();
        uint8_t sh = (uint8_t)startTime.substring(11, 13).toInt();
        uint8_t sm = (uint8_t)startTime.substring(14, 16).toInt();
        uint8_t eh = (uint8_t)endTime.substring(11, 13).toInt();
        uint8_t em = (uint8_t)endTime.substring(14, 16).toInt();
        employeeStartMinutes[count] = sh * 60 + sm;
        employeeEndMinutes[count]   = eh * 60 + em;
      } else {
        employeeData[count][2] = "";
        employeeScheduleDay[count] = 0;
        employeeStartMinutes[count] = 0;
        employeeEndMinutes[count] = 0;
      }
    } else {
      employeeData[count][2] = "";
      employeeScheduleDay[count] = 0;
      employeeStartMinutes[count] = 0;
      employeeEndMinutes[count] = 0;
    }
    count++;
  }

  employeeCount = count;
  Serial.printf("Supabase: Loaded %d employees\r\n", employeeCount);
  return employeeCount > 0;
}
