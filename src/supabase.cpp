#include "supabase.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

SupabaseClient::SupabaseClient(const char* u, const char* k, const SupabaseFieldMapping& fm, uint32_t ms)
  : url(u), apiKey(k), fields(fm), interval(ms) {}

bool SupabaseClient::fetch(EmployeeStore& store, int max) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Supabase fetch skipped: WiFi not connected");
    return false;
  }

  WiFiClientSecure secureClient;
  secureClient.setInsecure();

  HTTPClient http;
  http.setTimeout(10000);
  http.begin(secureClient, url);
  http.addHeader("apikey", apiKey);
  http.addHeader("Authorization", String("Bearer ") + apiKey);
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

  store.clear();
  JsonArray employees = doc.as<JsonArray>();
  int count = 0;

  for (JsonObject emp : employees) {
    if (count >= max) break;

    Employee e;
    String firstName = emp[fields.firstName] | "";
    String lastName  = emp[fields.lastName]  | "";
    e.lines[0] = firstName + " " + lastName;
    e.lines[1] = emp[fields.position] | "";

    const char* startStr = emp[fields.start];
    const char* endStr   = emp[fields.end];

    if (startStr && endStr) {
      String st = String(startStr);
      String et = String(endStr);
      if (st.length() >= 16 && et.length() >= 16) {
        e.lines[2] = st.substring(11, 16) + " - " + et.substring(11, 16);
        e.scheduleDay = (uint8_t)st.substring(8, 10).toInt();
        uint8_t sh = (uint8_t)st.substring(11, 13).toInt();
        uint8_t sm = (uint8_t)st.substring(14, 16).toInt();
        uint8_t eh = (uint8_t)et.substring(11, 13).toInt();
        uint8_t em = (uint8_t)et.substring(14, 16).toInt();
        e.startMin = sh * 60 + sm;
        e.endMin   = eh * 60 + em;
      }
    }

    store.push(e);
    count++;
  }

  Serial.printf("Supabase: Loaded %d employees\r\n", store.size());
  return store.size() > 0;
}

bool SupabaseClient::shouldFetch(unsigned long now) const {
  return now - lastFetch >= interval;
}

void SupabaseClient::markFetched(unsigned long now) {
  lastFetch = now;
}
