#include "employee.h"

bool EmployeeStore::push(const Employee& e) {
  if (count >= MAX_EMPLOYEES) return false;
  data[count++] = e;
  return true;
}

const Employee& EmployeeStore::get(int i) const {
  return data[i];
}

int EmployeeStore::collectActive(uint8_t today, uint16_t nowMin, int* out, int max) const {
  int n = 0;
  for (int i = 0; i < count && n < max; i++) {
    if (data[i].isActive(today, nowMin)) {
      out[n++] = i;
    }
  }
  return n;
}
