#include "utils.h"
#include <sstream>
#include <cmath>

// split a comma delimited string into a vector of strings
std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems)
{
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim))
  {
    elems.push_back(item);
  }
  return elems;
}

//return the day of year from the month and day of month
// month - (1-12)
// day in month (1-31)
// return -1 if month or dom are outof range
// dom is not checked for each month so e.g. February 30th will return a value
int doyFromMonthAndDay(int month, int dom)
{
  int som[12] =    /* start of month - 1 (day-of-year) */
  { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
  if (month < 1 || month > 12)
    return -1;
  if (dom < 1 || dom > 31)
    return -1;

  return (short)(som[month - 1] + dom);
}

/***  satpt.c  ***************************************************************/
/*  Compute saturation pressure [Pa] at temperature x [C].
*  Uses detailed curve fit from 1985 ASHRAE Fundamentals, chapter 6,
*  equations 3 and 4.  */
float satpt(float x)
{
  float y;
  double plog;

  if (x < 203.0)
    x = 203.0f;
  if (x > 373.0)
    x = 373.0f;
  if (x < 273.16)
    plog = x * (-9.677843e-3 + x * (6.2215701e-7 +
      x * (2.0747825e-9 - x * 9.484024e-13))) +
    4.1635019 * log(x) - 5.6745359e3 / x + 6.3925247;
  else
    plog = x * (-4.8640239e-2 + x * (4.1764768e-5 - x * 1.4452093e-8)) +
    6.5459673 * log(x) - 5.8002206e3 / x + 1.3914993;
  y = (float)exp(plog);

  return y;

}  /* end of satpt */

   /***  psywdp.c  **************************************************************/
   /*  psywdp:  compute humidity ratio (-) */
float psywdp(float Td, float Pb)
/*  Td  dew point temperature [K]
*  Pb  absolute barometric pressure [Pa]
*/
{
  const float MasRat = 0.62472f;  /* water / air adjusted by 1.0044 */
  float Pdew, w;
  Pdew = satpt(Td);
  w = Pdew * MasRat / (Pb - Pdew);
  return (w);
}  /* end of psywdp */

   /***  skyTf.c  ***************************************************************/
   /*  Estimate sky temperature  */
float skyTf(float Ta, float Td, float tcc)
/*  Ta;  dry bulb temperature [K].
*  Td;  dew point temperature [K].
*  tcc; tenths cloud cover.  */
{
  float Ts;
  double es;  /* effective sky emissivity */

  es = (0.787 + 0.764 * log(Td / 273.15)) *
    (1.0 + tcc * (0.0224 + tcc * (-0.0035 + 0.00028 * tcc)));
  Ts = Ta * (float)sqrt(sqrt(es));

  return Ts;

}  /* end skyTf */

// convert a day of the year to a dateX string e.g. 1/1
// return an empty string if the date given is out of range
std::string IntDateXToStringDateX(int Date)
{
  std::string stringDate;
  int month, day_of_month = 0;
  int som[12] =    /* start of month - 1 (day-of-year) */
  { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };

  if (Date < 1 || Date > 365)
    return "";

  for (month = 1; month < 12; month++)
  {
    if (som[month] >= Date) break;
  }
  day_of_month = Date - som[month - 1];

  stringDate = std::to_string(month) + "/" + std::to_string(day_of_month);

  return stringDate;
}

// convert a seconds in the day time to a string time (HH:MM:SS)
// return an empty string if the time value is out of range
std::string IntTimeToStringTime(int time)
{
  std::string strHours, strMinutes, strSeconds;
  int intHours, intMinutes, intSeconds;

  if (time < 0 || time > 86400)
    return"";

  intMinutes = time / 60;
  intSeconds = time % 60;
  intHours = intMinutes / 60;
  intMinutes = intMinutes % 60;
  if (intHours < 10)
    strHours = "0" + std::to_string(intHours);
  else
    strHours = std::to_string(intHours);
  if (intSeconds < 10)
    strSeconds = "0" + std::to_string(intSeconds);
  else
    strSeconds = std::to_string(intSeconds);
  if (intMinutes < 10)
    strMinutes = "0" + std::to_string(intMinutes);
  else
    strMinutes = std::to_string(intMinutes);
  return strHours + ":" + strMinutes + ":" + strSeconds;
}

//convert a dateX string (1/1) to a day of the year
// if a valid date cannot be determined then return -1
int StringDateXToIntDateX(std::string Date)
{
  int c, day = 0, month = 0;
  int lom[12] =    /* length of month - no leap year */
  { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  int som[12] =    /* start of month - 1 (day-of-year) */
  { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };


  c = Date.find("/", 0);
  // if slash is not found
  if (c == std::string::npos)
  {
    return -1;
  }
  month = std::stoi(Date.substr(0, c));
  day = std::stoi(Date.substr(c + 1, Date.length() - (c + 1)));

  // if date is out of range
  if (month < 1 || month > 12 || day < 1 || day > lom[month - 1])
  {
    return -1;
  }
  return (short)(som[month - 1] + day);
}

// get a double field from the JSON object given
// fieldName - the name of the field to get from the JSON object
// return infinity if the field is not found or field is not a number
double getDoubleFromJSON(std::string fieldName, const cJSON *JSONObject)
{
  const cJSON *JSONField = NULL;
  JSONField = cJSON_GetObjectItem(JSONObject, fieldName.c_str());
  if (cJSON_IsNumber(JSONField))
  {
    return JSONField->valuedouble;
  }
  else
  {
    return std::numeric_limits<double>::infinity();
  }
}

// get a float field from the JSON object given
// fieldName - the name of the field to get from the JSON object
// return infinity if the field is not found or field is not a number
float getFloatFromJSON(std::string fieldName, const cJSON *JSONObject)
{
  const cJSON *JSONField = NULL;
  JSONField = cJSON_GetObjectItem(JSONObject, fieldName.c_str());
  if (cJSON_IsNumber(JSONField))
  {
    return (float)JSONField->valuedouble;
  }
  else
  {
    return std::numeric_limits<float>::infinity();
  }
}

// get an integer field from the JSON object given
// fieldName - the name of the field to get from the JSON object
// return INT_MAX if the field is not found or field is not a number
int getIntFromJSON(std::string fieldName, const cJSON *JSONObject)
{
  const cJSON *JSONField = NULL;
  JSONField = cJSON_GetObjectItem(JSONObject, fieldName.c_str());
  if (cJSON_IsNumber(JSONField))
  {
    return JSONField->valueint;
  }
  else
  {
    return std::numeric_limits<int>::max();
  }
}

// get a string field from the JSON object given
// fieldName - the name of the field to get from the JSON object
// return empty string if the field is not found or field is not a string
std::string getStringFromJSON(std::string fieldName, const cJSON *JSONObject)
{
  const cJSON *JSONField = NULL;
  JSONField = cJSON_GetObjectItem(JSONObject, fieldName.c_str());
  if (cJSON_IsString(JSONField) && (JSONField->valuestring != NULL))
  {
    return JSONField->valuestring;
  }
  else
  {
    return std::string();
  }

}

// return 1 if the dateToTest is within the range of dates given
int dateIsWithinRange(int dateToTest, int rangeStart, int rangeEnd)
{
  if (rangeStart <= rangeEnd)
  {
    return dateToTest >= rangeStart && dateToTest <= rangeEnd;
  }
  else
  {
    return (dateToTest >= rangeStart && dateToTest <= 365) ||
      (dateToTest >= 1 && dateToTest <= rangeEnd);
  }
}
