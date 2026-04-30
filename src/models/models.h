#pragma once
#include <string>
#include <vector>

enum class Acuity { CRITICAL = 1, HIGH = 2, MEDIUM = 3, LOW = 4 };
enum class Role   { PGY1, INTERN, NURSE };
enum class Status { PENDING, DONE };

struct Task {
  std::string id;
  std::string name;
  Role        required_role;
  int         duration_minutes;
  int         priority;          
  std::string assigned_worker;   
  Status      status = Status::PENDING;
};
struct Patient {
  std::string id;
  std::string name;
  int         age;
  std::string complaint;
  Acuity      acuity;
  std::string disease_id;        
  std::vector<std::string> test_overrides;
};
struct Worker {
  std::string id;
  std::string name;
  Role        role;
  int         shift_minutes_left;
  int         available_at = 0; 
};
struct Disease {
  std::string id;
  std::string name;
  int         base_priority;
  std::vector<std::string> test_ids;
};
