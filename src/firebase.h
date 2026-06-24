#ifndef FIREBASE_H
#define FIREBASE_H

#include "config.h"
#include "employee.h"

class FirebaseClient {
  const char* url;
  const char* clientEmail;
  const char* privateKeyPem;
  SupabaseFieldMapping fields;
  uint32_t interval;
  unsigned long lastFetch = 0;

  // Cached OAuth2 Token
  String cachedToken;
  uint32_t tokenExpireEpoch = 0;

  void parseScheduleTimes(Employee& e, const char* startStr, const char* endStr);
  
  // RSA-256 JWT Generation and Google Token Exchange helpers
  String base64urlEncode(const uint8_t* data, size_t length);
  String signRS256(const String& message, const String& privateKeyPem);
  String getAccessToken(uint32_t nowEpoch);

public:
  FirebaseClient(const char* url, const char* clientEmail, const char* privateKeyPem, const SupabaseFieldMapping& fm, uint32_t fetchMs);

  bool fetch(EmployeeStore& store, int max, uint32_t nowEpoch);
  bool shouldFetch(unsigned long now) const;
  void markFetched(unsigned long now);
  void markFailed(unsigned long now);
};

#endif
