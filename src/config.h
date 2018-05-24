#pragma once
#include "../lib/cJSON/cJSON.h"
#include <string>
#include <vector>

// this holds the data for a special day
struct specialDayStruct
{
  int date;  // this is the doy for this special day
  int dtype; // this is the daytpe for this special day
};

// this holds the config data 
struct configStruct 
{
  int startDate;      // the doy to start weather data (1-365) (-1 means use the EPW start date)
  int endDate;        // the doy to end weather data (1-365) (-1 means use the EPW end date)
  std::string descr;  // the description for the weather file
  int useDST;         // indicates whether to use daylight savings time (DST) (0 = false, 1 = true)
  int startDateDST;   // the doy to start DST (1-365)
  int endDateDST;     // the doy to end DST (1-365)
  int firstDOY;       // indicates which day to the week is the first doy (1-7)
  int validConfig;    // this indicates if the config was processed correctly 
  std::string errMsg; // error message for when validConfig is false
  std::vector<specialDayStruct> specialDays;
};

configStruct getConfigData(cJSON *cnfJSON);
