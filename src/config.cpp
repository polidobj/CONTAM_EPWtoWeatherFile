#include "config.h"
#include "utils.h"

// extract the config data from the JSON 
// and return it in the config struct
configStruct getConfigData(cJSON *cnfJSON)
{
  configStruct config;

  // initialize validConfig to true
  config.validConfig = 1;

  // get the start date in the config file
  std::string cnfStartDateString = getStringFromJSON("startdate", cnfJSON);
  if (cnfStartDateString.length() == 0)
  {
    config.errMsg = "startdate not found in the config file.";
    config.validConfig = 0;
    return config;
  }
  config.startDate = StringDateXToIntDateX(cnfStartDateString);
  if (config.startDate == -1)
  {
    config.errMsg = "Invalid start date in the config file: " + cnfStartDateString;
    config.validConfig = 0;
    return config;
  }

  // get the end date in the config file
  std::string cnfEndDateString = getStringFromJSON("enddate", cnfJSON);
  if (cnfEndDateString.length() == 0)
  {
    config.errMsg = "enddate not found in the config file.";
    config.validConfig = 0;
    return config;
  }
  config.endDate = StringDateXToIntDateX(cnfEndDateString);
  if (config.endDate == -1)
  {
    config.errMsg = "Invalid end date in the config file: " + cnfEndDateString;
    config.validConfig = 0;
    return config;
  }

  // empty description is ok
  config.descr = getStringFromJSON("description", cnfJSON);
  
  config.useDST = getIntFromJSON("usedst", cnfJSON);
  if (config.useDST == std::numeric_limits<int>::max())
  {
    config.errMsg = "Invalid useDST in the config file: " + std::to_string(config.useDST);
    config.validConfig = 0;
    return config;
  }

  if(config.useDST)
  {
    // get the start date DST in the config file
    std::string cnfStartDateDSTString = getStringFromJSON("dststart", cnfJSON);
    if (cnfStartDateDSTString.length() == 0)
    {
      config.errMsg = "dststart not found in the config file.";
      config.validConfig = 0;
      return config;
    }
    config.startDateDST = StringDateXToIntDateX(cnfStartDateDSTString);
    if (config.startDateDST == -1)
    {
      config.errMsg = "Invalid dststart in the config file: " + cnfStartDateDSTString;
      config.validConfig = 0;
      return config;
    }

    // get the end date DST in the config file
    std::string cnfEndDateDSTString = getStringFromJSON("dstend", cnfJSON);
    if (cnfEndDateDSTString.length() == 0)
    {
      config.errMsg = "dstend not found in the config file.";
      config.validConfig = 0;
      return config;
    }
    config.endDateDST = StringDateXToIntDateX(cnfEndDateDSTString);
    if (config.endDateDST == -1)
    {
      config.errMsg = "Invalid dstend in the config file: " + cnfEndDateDSTString;
      config.validConfig = 0;
      return config;
    }
  }

  config.firstDOY = getIntFromJSON("firstdoy", cnfJSON);
  if (config.firstDOY == std::numeric_limits<int>::max() ||
    config.firstDOY < 1 || config.firstDOY > 7)
  {
    config.errMsg = "Invalid firstDOY in the config file: " + std::to_string(config.firstDOY);
    config.validConfig = 0;
    return config;
  }

  // process the special days
  const cJSON *specialDays = NULL;
  const cJSON *specialDay = NULL;
  specialDays = cJSON_GetObjectItem(cnfJSON, "specialdays");
  cJSON_ArrayForEach(specialDay, specialDays)
  {
    specialDayStruct sd;

    std::string dateString = getStringFromJSON("date", specialDay);
    if (dateString.empty())
    {
      config.errMsg = "date not found in a special day object";
      config.validConfig = 0;
      return config;
    }
    sd.date = StringDateXToIntDateX(dateString);
    if (sd.date == -1)
    {
      config.errMsg = "Invalid date in the special day: " + dateString;
      config.validConfig = 0;
      return config;
    }

    sd.dtype = getIntFromJSON("daytype", specialDay);
    if (sd.dtype == std::numeric_limits<int>::max() ||
      sd.dtype < 1 || sd.dtype > 12)
    {
      config.errMsg = "Invalid day type in special day: " + std::to_string(sd.dtype);
      config.validConfig = 0;
      return config;
    }

    config.specialDays.push_back(sd);
  }

  return config;
}
