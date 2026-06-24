#ifndef SUPABASE_H
#define SUPABASE_H

#include "config.h"
#include "employee.h"

class SupabaseClient {
  const char* url;
  const char* apiKey;
  SupabaseFieldMapping fields;
  uint32_t interval;
  unsigned long lastFetch = 0;

public:
  SupabaseClient(const char* url, const char* key, const SupabaseFieldMapping& fm, uint32_t fetchMs);

  bool fetch(EmployeeStore& store, int max, uint32_t nowEpoch = 0);
  bool shouldFetch(unsigned long now) const;
  void markFetched(unsigned long now);
  void markFailed(unsigned long now);
};

#endif
