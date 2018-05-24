#pragma once
#include "../lib/cJSON/cJSON.h"
#include <string>
#include <vector>

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems);
int doyFromMonthAndDay(int month, int dom);
float satpt(float x);
float psywdp(float Td, float Pb);
float skyTf(float Ta, float Td, float tcc);
std::string IntDateXToStringDateX(int Date);
std::string IntTimeToStringTime(int time);
int StringDateXToIntDateX(std::string Date);
double getDoubleFromJSON(std::string fieldName, const cJSON *JSONObject);
float getFloatFromJSON(std::string fieldName, const cJSON *JSONObject);
int getIntFromJSON(std::string fieldName, const cJSON *JSONObject);
std::string getStringFromJSON(std::string fieldName, const cJSON *JSONObject);
int dateIsWithinRange(int dateToTest, int rangeStart, int rangeEnd);
