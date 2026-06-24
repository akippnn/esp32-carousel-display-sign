#ifndef EMPLOYEE_H
#define EMPLOYEE_H

#include <Arduino.h>
#include "config.h"

struct Employee {
  String lines[3];
  uint8_t scheduleDay = 0;
  uint16_t startMin = 0;
  uint16_t endMin = 0;
  bool checkedIn = false;

  bool isActive(uint8_t today, uint16_t nowMin) const {
    return scheduleDay == today && nowMin >= startMin && nowMin < endMin;
  }
};

class EmployeeStore {
  Employee data[MAX_EMPLOYEES];
  int count = 0;

public:
  void clear() { count = 0; }
  bool push(const Employee& e);
  int size() const { return count; }
  bool empty() const { return count == 0; }
  const Employee& get(int i) const;
  Employee& getMutable(int i) { return data[i]; }
  int collectActive(uint8_t today, uint16_t nowMin, int* out, int max) const;
};

#endif
