#include "EPWConverter.h"
#include "utils.h"

#include <string>
#include <vector>
#include <iostream>

// output the day type definitions in the header section of the weather file
void outputWthHeaderDays(int startDate, int endDate, std::ostream &ostream,
  int &dayOfWeek, int &dayType, configStruct config)
{
  for (int currentDate = startDate; currentDate <= endDate; ++currentDate)
  {
    // check if DST should be use for the current date
    int daylightSavings;
    if (config.useDST && dateIsWithinRange(currentDate, config.startDateDST, config.endDateDST))
    {
      daylightSavings = 1;
    }
    else
    {
      daylightSavings = 0;
    }

    //check for special days
    int dType = dayType;
    for (size_t index = 0; index < config.specialDays.size(); ++index)
    {
      if (currentDate == config.specialDays[index].date)
      {
        dType = config.specialDays[index].dtype;
      }
    }

    ostream << IntDateXToStringDateX(currentDate) << '\t'
      << dayOfWeek << '\t' << dType << '\t'
      << daylightSavings << '\t' << "283.15" << std::endl;
    dayOfWeek++;
    if (dayOfWeek > 7)
    {
      dayOfWeek = 1;
    }

    dayType++;
    if (dayType > 7)
    {
      dayType = 1;
    }
  }
}

// read a line of data from the epw file
// output a corresponding line to the wth file
// lineitems - a vector of strings for a line of data from the epw file
// ostream - the output stream to write data to the wth file
// firstRecord - this is true if this is the first record of data to be processed
// startDate - the date to start outputing data
// endDate - the date to end outputing data
void processDataLine(std::vector<std::string> &lineItems, std::ostream &ostream, 
  bool &firstRecord, int startDate, int endDate)
{
  /*    The EE values are stored as comma delimited data --
  *       field        description
  *          1         year (4 digit)
  *          2         month (1 - 12)
  *          3         day (1 - 31)
  *          4         hour (1 - 24)
  *          5         minute (1 - 60)
  *          7         dry bulb temperature [C]
  *          8         dew point temperature [C]
  *         10         station pressure [Pa]
  *         13         infrared radiation (horizontal) [Wh/m^2]
  *         14         solar radiation (horizontal) [Wh/m^2]
  *         15         solar radiation (direct normal) [Wh/m^2]
  *         21         wind direction [degrees]
  *         22         wind speed [m/s]
  *         27?        rain (if = 1 thru 8)
  *         31         snow depth [cm]
  */
  // the values above are 1 based
  // the values below are 0 based

  // get columns that we need from the data
  // and do calculations as needed to get the items we need for the wth file
  int month = std::stoi(lineItems[1]);
  int day = std::stoi(lineItems[2]);
  int doy = doyFromMonthAndDay(month, day);

  // if the doy for this record is not in range then skip it
  if (!dateIsWithinRange(doy, startDate, endDate))
  {
    return;
  }

  int hour = std::stoi(lineItems[3]);
  int minute = std::stoi(lineItems[4]);
  //compute the number of seconds of the day
  int time = hour * 3600 + minute * 60;

  float dryBuldTemperatureC = std::stof(lineItems[6]);
  float dryBulbTemperatureK = 273.15f + dryBuldTemperatureC;
  float dewPointTemperatureC = std::stof(lineItems[7]);
  float dewPointTemperatureK = 273.15f + dewPointTemperatureC;
  float barometricPressure = std::stof(lineItems[9]);
  float windSpeed = std::stof(lineItems[21]);
  float windDirection = std::stof(lineItems[20]);
  float humidityRaito = 1000.0f * psywdp(dewPointTemperatureK, barometricPressure);

  float totalHorizontalSolarRadiation1 = std::stof(lineItems[13]);
  float totalHorizontalSolarRadiation2 = 3.6f * totalHorizontalSolarRadiation1; /* [Wh/m^2] to [kJ/m^2] */
  float directNormalSolarRadiation1 = std::stof(lineItems[14]);
  float directNormalSolarRadiation2 = 3.6f * directNormalSolarRadiation1; /* [Wh/m^2] to [kJ/m^2] */
  float tenthsCloudCover = 0.0;
  float totalSkyCover = std::stof(lineItems[22]);
  if (totalSkyCover != 99)
    tenthsCloudCover = 0.1f * totalSkyCover;
  float skyRadiantTemprerature = skyTf(dryBulbTemperatureK, dewPointTemperatureK, tenthsCloudCover);

  std::string WeatherCodes = lineItems[27];
  int rainCode = std::stoi(WeatherCodes.substr(1, 1));
  int rain;
  if (rainCode >= 1 && rainCode <= 8)
    rain = 1;
  else
    rain = 0;

  int snow;
  int snowDepth = std::stoi(lineItems[30]);
  if (snowDepth > 1)
    snow = 1;
  else
    snow = 0;

  // if this is the first record of data and the time != 0
  if (firstRecord && time != 0)
  {
    // output the same data but with time == 0 since CONTAM requires the first line to be time == 0
    ostream << IntDateXToStringDateX(doy) << '\t' << IntTimeToStringTime(0) << '\t'
      << dryBulbTemperatureK << '\t' << barometricPressure << '\t'
      << windSpeed << '\t' << windDirection << '\t' << humidityRaito << '\t'
      << totalHorizontalSolarRadiation2 << '\t' << directNormalSolarRadiation2 << '\t'
      << skyRadiantTemprerature << '\t' << rain << '\t' << snow << std::endl;
  }

  // write data to wth file
  ostream << IntDateXToStringDateX(doy) << '\t' << IntTimeToStringTime(time) << '\t'
    << dryBulbTemperatureK << '\t' << barometricPressure << '\t'
    << windSpeed << '\t' << windDirection << '\t' << humidityRaito << '\t'
    << totalHorizontalSolarRadiation2 << '\t' << directNormalSolarRadiation2 << '\t'
    << skyRadiantTemprerature << '\t' << rain << '\t' << snow << std::endl;
  // a record has been output
  firstRecord = false;

}

// convert an EPW file to a CONTAM Weather file
// config - a struct representation of the config file
// istream - a stream that contains the epw file
// ostream - the stream where the CONTAM weather file will be output
// both streams are assumed to be opened
int convertEPW(configStruct config, std::istream &istream, std::ostream &ostream)
{
  std::string line;
  std::vector<std::string> lineItems;
  std::string epwDescription;

  // get line 1 location data
  // use it as the default description for the weather file
  std::getline(istream, line);
  // truncate since description is limited to 256 chars in CONTAM weather files
  epwDescription = line.substr(0, 256);

  // get line 2 conditions
  std::getline(istream, line);

  // get line 3 preiods
  std::getline(istream, line);

  // get line 4 ground temps
  std::getline(istream, line);

  // get line 5 holidays
  std::getline(istream, line);

  // get line 6 comments #1
  std::getline(istream, line);

  // get line 7 comments #2
  std::getline(istream, line);

  // get line 8 data periods
  std::getline(istream, line);

  // split the line by comma
  split(line, ',', lineItems);

  // get the start and end dates for the EPW file
  std::string epwStartDateString = lineItems[5];
  std::string epwEndDateString = lineItems[6];
  int epwStartDate = StringDateXToIntDateX(epwStartDateString);
  if (epwStartDate == -1)
  {
    std::cerr << "Invalid start date in the epw file: " << epwStartDateString << std::endl;
    return -1;
  }
  int epwEndDate = StringDateXToIntDateX(epwEndDateString);
  if (epwEndDate == -1)
  {
    std::cerr << "Invalid end date in the epw file: " << epwEndDate << std::endl;
    return -1;
  }

  //check if a config was given
  if (config.startDate > -1 && config.endDate > -1)
  {
    //ensure that the config start and end dates are found in the EPW file
    if (!dateIsWithinRange(config.startDate, epwStartDate, epwEndDate))
    {
      std::cerr << "The start date in config is not within the dates included in the EPW file." << std::endl;
      return -1;
    }
    if (!dateIsWithinRange(config.endDate, epwStartDate, epwEndDate))
    {
      std::cerr << "The end date in config is not within the dates included in the EPW file." << std::endl;
      return -1;
    }
  }


  // if the config contains a description use that 
  // otherwise use the first line of the epw file
  std::string description;
  if (config.descr.length() == 0)
    description = epwDescription;
  else
    description = config.descr;

  //determine the start and end dates
  // if the config has -1 for the dates then use the EPW date
  int startDate;
  if (config.startDate == -1)
  {
    startDate = epwStartDate;
  }
  else
  {
    startDate = config.startDate;
  }
  int endDate;
  if (config.endDate == -1)
  {
    endDate = epwEndDate;
  }
  else
  {
    endDate = config.endDate;
  }

  //write weather file head section
  ostream << "WeatherFile ContamW 2.0" << std::endl
    << description << std::endl
    << IntDateXToStringDateX(startDate) << " !start - of - file date" << std::endl
    << IntDateXToStringDateX(endDate) << " !end - of - file date" << std::endl
    << "!Date" << '\t' << "DofW" << '\t' << "Dtype" << '\t'
    << "DST" << '\t' << "Tgrnd [K]" << std::endl;

  int dayType = config.firstDOY;
  int dayOfWeek = config.firstDOY;

  if (startDate <= endDate)
  {
    outputWthHeaderDays(startDate, endDate, ostream, dayOfWeek, dayType, config);
  }
  else
  {
    outputWthHeaderDays(startDate, 365, ostream, dayOfWeek, dayType, config);
    outputWthHeaderDays(1, endDate, ostream, dayOfWeek, dayType, config);
  }

  // write comment line that describes the data columns
  ostream << "!Date" << '\t' << "Time" << '\t' << "Ta[K]" << '\t' << "Pb[Pa]" << '\t' 
    << "Ws[m / s]" << '\t' << "Wd[deg]" << '\t' << "Hr[g / kg]" << '\t' 
    << "Ith[kJ / m ^ 2]" << '\t' << "Idn[kJ / m ^ 2]" << '\t' << "Ts[K]" << '\t' 
    << "Rn[-]" << '\t' << "Sn[-]" << std::endl;

  // this starts true and stays true until the first record is output
  bool firstRecord = true;

  // get the first line of data
  std::getline(istream, line);
  while (line.length() > 0)
  {
    // split the line by comma
    lineItems.clear();
    split(line, ',', lineItems);

    processDataLine(lineItems, ostream, firstRecord, startDate, endDate);

    // get the next line of data
    std::getline(istream, line);
  }

  return 0;
}
