#include "firebase.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "mbedtls/pk.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/md.h"

FirebaseClient::FirebaseClient(const char* u, const char* email, const char* key, const SupabaseFieldMapping& fm, uint32_t ms)
  : url(u), clientEmail(email), privateKeyPem(key), fields(fm), interval(ms) {}

void FirebaseClient::parseScheduleTimes(Employee& e, const char* startStr, const char* endStr) {
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
}

String FirebaseClient::base64urlEncode(const uint8_t* data, size_t length) {
  static const char base64_chars[] = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789-_";

  String encoded;
  encoded.reserve(((length + 2) / 3) * 4);

  int i = 0;
  int j = 0;
  uint8_t char_array_3[3];
  uint8_t char_array_4[4];

  while (length--) {
    char_array_3[i++] = *(data++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for (i = 0; i < 4; i++) {
        encoded += base64_chars[char_array_4[i]];
      }
      i = 0;
    }
  }

  if (i) {
    for (j = i; j < 3; j++) {
      char_array_3[j] = '\0';
    }

    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
    char_array_4[3] = char_array_3[2] & 0x3f;

    for (j = 0; (j < i + 1); j++) {
      encoded += base64_chars[char_array_4[j]];
    }
  }

  return encoded;
}

String FirebaseClient::signRS256(const String& message, const String& privateKey) {
  String decodedKey = privateKey;
  decodedKey.replace("\\n", "\n");

  mbedtls_pk_context pk;
  mbedtls_entropy_context entropy;
  mbedtls_ctr_drbg_context ctr_drbg;

  mbedtls_pk_init(&pk);
  mbedtls_entropy_init(&entropy);
  mbedtls_ctr_drbg_init(&ctr_drbg);

  const char* pers = "oauth2_jwt_signing";
  int ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const uint8_t*)pers, strlen(pers));
  if (ret != 0) {
    Serial.printf("mbedtls_ctr_drbg_seed failed: -0x%04X\r\n", -ret);
    mbedtls_pk_free(&pk);
    mbedtls_entropy_free(&entropy);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    return "";
  }

  ret = mbedtls_pk_parse_key(&pk, (const uint8_t*)decodedKey.c_str(), decodedKey.length() + 1, NULL, 0);
  if (ret != 0) {
    Serial.printf("mbedtls_pk_parse_key failed: -0x%04X\r\n", -ret);
    mbedtls_pk_free(&pk);
    mbedtls_entropy_free(&entropy);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    return "";
  }

  uint8_t hash[32];
  ret = mbedtls_md(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), (const uint8_t*)message.c_str(), message.length(), hash);
  if (ret != 0) {
    Serial.printf("mbedtls_md failed: -0x%04X\r\n", -ret);
    mbedtls_pk_free(&pk);
    mbedtls_entropy_free(&entropy);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    return "";
  }

  uint8_t signature[512];
  size_t sig_len = 0;
  ret = mbedtls_pk_sign(&pk, MBEDTLS_MD_SHA256, hash, sizeof(hash), signature, &sig_len, mbedtls_ctr_drbg_random, &ctr_drbg);
  if (ret != 0) {
    Serial.printf("mbedtls_pk_sign failed: -0x%04X\r\n", -ret);
    mbedtls_pk_free(&pk);
    mbedtls_entropy_free(&entropy);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    return "";
  }

  String encodedSignature = base64urlEncode(signature, sig_len);

  mbedtls_pk_free(&pk);
  mbedtls_entropy_free(&entropy);
  mbedtls_ctr_drbg_free(&ctr_drbg);

  return encodedSignature;
}

String FirebaseClient::getAccessToken(uint32_t nowEpoch) {
  if (clientEmail == nullptr || strlen(clientEmail) == 0 || privateKeyPem == nullptr || strlen(privateKeyPem) == 0) {
    Serial.println("Google OAuth2: Client email or private key is missing.");
    return "";
  }

  String payload = "{\"iss\":\"" + String(clientEmail) + "\",";
  payload += "\"scope\":\"https://www.googleapis.com/auth/userinfo.email https://www.googleapis.com/auth/firebase.database\",";
  payload += "\"aud\":\"https://oauth2.googleapis.com/token\",";
  payload += "\"exp\":" + String(nowEpoch + 3600) + ",";
  payload += "\"iat\":" + String(nowEpoch) + "}";

  String header_b64 = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9";
  String payload_b64 = base64urlEncode((const uint8_t*)payload.c_str(), payload.length());
  String jwt_message = header_b64 + "." + payload_b64;

  String signature_b64 = signRS256(jwt_message, privateKeyPem);
  if (signature_b64.length() == 0) {
    Serial.println("Google OAuth2: JWT signing failed.");
    return "";
  }

  String jwt = jwt_message + "." + signature_b64;

  WiFiClientSecure secureClient;
  secureClient.setInsecure();

  HTTPClient http;
  http.setTimeout(15000);
  http.begin(secureClient, "https://oauth2.googleapis.com/token");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  String postBody = "grant_type=urn%3Aietf%3Aparams%3Aoauth%3Agrant-type%3Ajwt-bearer&assertion=" + jwt;
  int httpCode = http.POST(postBody);

  if (httpCode != 200) {
    Serial.printf("Google OAuth2 token exchange failed: HTTP %d\r\n", httpCode);
    String errorResponse = http.getString();
    Serial.printf("Response: %s\r\n", errorResponse.c_str());
    http.end();
    return "";
  }

  String response = http.getString();
  http.end();

#if ARDUINOJSON_VERSION_MAJOR >= 7
  JsonDocument doc;
#else
  DynamicJsonDocument doc(1024);
#endif

  DeserializationError error = deserializeJson(doc, response);
  if (error) {
    Serial.printf("Google OAuth2 JSON parse failed: %s\r\n", error.c_str());
    return "";
  }

  const char* token = doc["access_token"];
  uint32_t expires_in = doc["expires_in"] | 3600;

  if (!token) {
    Serial.println("Google OAuth2 response did not contain access_token.");
    return "";
  }

  cachedToken = String(token);
  tokenExpireEpoch = nowEpoch + expires_in - 60; // Refresh 1 minute early to be safe
  Serial.println("Google OAuth2: Access token fetched and cached successfully.");
  return cachedToken;
}

bool FirebaseClient::fetch(EmployeeStore& store, int max, uint32_t nowEpoch) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Firebase fetch skipped: WiFi not connected");
    return false;
  }

  if (cachedToken.length() == 0 || nowEpoch >= tokenExpireEpoch) {
    Serial.println("Firebase: Access token expired or not available, fetching new one...");
    if (getAccessToken(nowEpoch).length() == 0) {
      Serial.println("Firebase: Token exchange failed. Aborting fetch.");
      return false;
    }
  }

  WiFiClientSecure secureClient;
  secureClient.setInsecure();

  HTTPClient http;
  http.setTimeout(10000);

  // Append .json to the URL if not already present
  String requestUrl = url;
  if (!requestUrl.endsWith(".json") && requestUrl.indexOf(".json?") == -1) {
    int qIdx = requestUrl.indexOf('?');
    if (qIdx >= 0) {
      requestUrl = requestUrl.substring(0, qIdx) + ".json" + requestUrl.substring(qIdx);
    } else {
      requestUrl += ".json";
    }
  }

  Serial.printf("Firebase: Fetching from %s\r\n", requestUrl.c_str());

  http.begin(secureClient, requestUrl);
  http.addHeader("Accept", "application/json");
  http.addHeader("Authorization", String("Bearer ") + cachedToken);

  int httpCode = http.GET();
  if (httpCode != 200) {
    Serial.printf("Firebase fetch failed: HTTP %d\r\n", httpCode);
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
    Serial.printf("Firebase JSON parse failed: %s\r\n", error.c_str());
    return false;
  }

  store.clear();
  int count = 0;

  if (doc.is<JsonArray>()) {
    JsonArray employees = doc.as<JsonArray>();
    for (JsonVariant val : employees) {
      if (count >= max) break;
      if (!val.is<JsonObject>()) continue;
      JsonObject emp = val.as<JsonObject>();
      
      Employee e;
      String firstName = emp[fields.firstName] | "";
      String lastName  = emp[fields.lastName]  | "";
      e.lines[0] = firstName + " " + lastName;
      e.lines[1] = emp[fields.position] | "";

      const char* startStr = emp[fields.start];
      const char* endStr   = emp[fields.end];
      parseScheduleTimes(e, startStr, endStr);

      store.push(e);
      count++;
    }
  } else if (doc.is<JsonObject>()) {
    JsonObject employees = doc.as<JsonObject>();
    for (JsonPair p : employees) {
      if (count >= max) break;
      if (!p.value().is<JsonObject>()) continue;
      JsonObject emp = p.value().as<JsonObject>();
      
      Employee e;
      String firstName = emp[fields.firstName] | "";
      String lastName  = emp[fields.lastName]  | "";
      e.lines[0] = firstName + " " + lastName;
      e.lines[1] = emp[fields.position] | "";

      const char* startStr = emp[fields.start];
      const char* endStr   = emp[fields.end];
      parseScheduleTimes(e, startStr, endStr);

      store.push(e);
      count++;
    }
  }

  Serial.printf("Firebase: Loaded %d employees\r\n", store.size());
  return store.size() > 0;
}

bool FirebaseClient::shouldFetch(unsigned long now) const {
  if (lastFetch == 0) return true;
  return now - lastFetch >= interval;
}

void FirebaseClient::markFetched(unsigned long now) {
  lastFetch = now;
}

void FirebaseClient::markFailed(unsigned long now) {
  lastFetch = now - interval + 5000;
}
