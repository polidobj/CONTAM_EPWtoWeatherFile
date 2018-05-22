#include "EPWConverter.h"
#include <string>
#include <vector>
#include <sstream>
#include <cmath>

// split a comma delimited string into a vector strings
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
int doyFromMonthAndDay(int month, int dom)
{
  int som[12] =    /* start of month - 1 (day-of-year) */
  { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };

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
std::string IntDateXToStringDateX(int Date)
{
  std::string stringDate;
  int month, day_of_month = 0;
  int som[12] =    /* start of month - 1 (day-of-year) */
  { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };

  for (month = 1; month < 12; month++)
  {
    if (som[month] >= Date) break;
  }
  day_of_month = Date - som[month - 1];

  stringDate = std::to_string(month) + "/" + std::to_string(day_of_month);

  return stringDate;
}

// convert a seconds in the day time to a string time (HH:MM:SS)
std::string IntTimeToStringTime(int time)
{
  std::string strHours, strMinutes, strSeconds;
  int intHours, intMinutes, intSeconds;

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
int StringDateXToIntDateX(std::string Date)
{
  int c, day = 0, month = 0;
  int lom[12] =    /* length of month - no leap year */
  { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  int som[12] =    /* start of month - 1 (day-of-year) */
  { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };


  c = Date.find("/", 0);
  month = std::stoi(Date.substr(0, c));
  day = std::stoi(Date.substr(c + 1, Date.length() - (c + 1)));

  if (month < 1 || month > 12 || day < 1 || day > lom[month - 1])
  {
    throw std::invalid_argument("Invalid date string given: " + Date);
  }
  return (short)(som[month - 1] + day);
}

void outputWthHeaderDays(int startDate, int endDate, std::ostream &ostream,
  int &dayOfWeek, int &dayType, int &daylightSavings, float tGround)
{
  for (int i = startDate; i <= endDate; ++i)
  {
    ostream << IntDateXToStringDateX(i) << '\t'
      << dayOfWeek << '\t' << dayType << '\t'
      << daylightSavings << '\t' << tGround << std::endl;
    dayOfWeek++;
    if (dayOfWeek > 7)
      dayOfWeek = 1;

    dayType++;
    if (dayType > 7)
      dayType = 1;
  }
}

// read a line of data from the epw file
// output a corresponding line to the wth file
// lineitems - a vector of strings for a line of data from the epw file
// ostream - the output stream to write data to the wth file
// firstRecord - this is true if this is the first record of data to be processed
void processDataLine(std::vector<std::string> &lineItems, std::ostream &ostream, bool firstRecord)
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

}


// convert an EPW file to a CONTAM Weather file
// options - a JSON string with the options for the conversion
// istream - a stream that contains the epw file
// ostream - the stream where the CONTAM weather file will be output
// both streams are assumed to be opened
int convertEPW(std::string options, std::istream &istream, std::ostream &ostream)
{
  std::string line;
  std::vector<std::string> lineItems;
  std::string description;

  // get line 1 location data
  // use it as the default description for the weather file
  std::getline(istream, line);
  // truncate since description is limited to 256 chars in CONTAM weather files
  description = line.substr(0, 256);

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

  std::string startDateString = lineItems[5];
  std::string endDateString = lineItems[6];
  int startDate = StringDateXToIntDateX(startDateString);
  int endDate = StringDateXToIntDateX(endDateString);
  int dayType = 1;
  int dayOfWeek = 1;
  int daylightSavings = 0;
  float tGround = 283.15f;

  //write weather file head section
  ostream << "WeatherFile ContamW 2.0" << std::endl
    << description << std::endl
    << IntDateXToStringDateX(startDate) << " !start - of - file date" << std::endl
    << IntDateXToStringDateX(endDate) << " !end - of - file date" << std::endl
    << "!Date" << '\t' << "DofW" << '\t' << "Dtype" << '\t'
    << "DST" << '\t' << "Tgrnd [K]" << std::endl;

  if (startDate <= endDate)
  {
    outputWthHeaderDays(startDate, endDate, ostream, dayOfWeek, dayType, daylightSavings, tGround);
  }
  else
  {
    outputWthHeaderDays(startDate, 365, ostream, dayOfWeek, dayType, daylightSavings, tGround);
    outputWthHeaderDays(1, endDate, ostream, dayOfWeek, dayType, daylightSavings, tGround);
  }

  // write comment line that describes the data columns
  ostream << "!Date" << '\t' << "Time" << '\t' << "Ta[K]" << '\t' << "Pb[Pa]" << '\t' 
    << "Ws[m / s]" << '\t' << "Wd[deg]" << '\t' << "Hr[g / kg]" << '\t' 
    << "Ith[kJ / m ^ 2]" << '\t' << "Idn[kJ / m ^ 2]" << '\t' << "Ts[K]" << '\t' 
    << "Rn[-]" << '\t' << "Sn[-]" << std::endl;

  // get the first line of data
  std::getline(istream, line);
  lineItems.clear();
  split(line, ',', lineItems);
  processDataLine(lineItems, ostream, true);

  // get the second line of data
  std::getline(istream, line);
  while (line.length() > 0)
  {
    // split the line by comma
    lineItems.clear();
    split(line, ',', lineItems);

    processDataLine(lineItems, ostream, false);

    // get the next line of data
    std::getline(istream, line);
  }

  return 0;
}
