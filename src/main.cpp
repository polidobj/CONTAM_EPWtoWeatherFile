#include "EPWConverter.h"
#include "../lib/cJSON/cJSON.h"
#include "config.h"

#include <iostream>
#include <string>
#include <fstream>

#ifndef EMSCRIPTEN

// program to convert an epw file to a CONTAM weather file
// the first argument is a path to a epw file to convert
// second srgument is a path to a CONTAM weather file to create (*.wth)
// the second argument is a path to a config file
//the config file contains JSON with options for the conversion
int main(int argc, char *argv[])
{
  // make sure that there is at least one param
  if (argc < 2)
  {
    std::cerr << "No command-line parameter given for epw file." << std::endl;
    return 1;
  }
  // make sure that there is at least two params
  if (argc < 3)
  {
    std::cerr << "No command-line parameter given for wth file." << std::endl;
    return 1;
  }

  // assume that the first param is the epw file path
  std::string epwPath = argv[1];
  // assume that the second param is the wth file path
  std::string wthPath = argv[2];
  bool cnfFilePresent = false;
  std::string cnfPath = "";
  configStruct config;

  // check for a third param
  if (argc > 3)
  {
    // assume that the third param is the config file path
    cnfPath = argv[3];
    cnfFilePresent = true;
  }

  //open streams
  std::ifstream epwStream;
  epwStream.open(epwPath);
  //check that the file was opened
  if (epwStream.fail())
  {
    std::cerr << "Failed to open the epw file: " << epwPath << std::endl;
    return 1;
  }

  if (cnfFilePresent)
  {
    std::string cnfFileContents;
    cJSON *cnfJSON;
    std::ifstream cnfStream;
    cnfStream.open(cnfPath);
    //check that the file was opened
    if (cnfStream.fail())
    {
      std::cerr << "Failed to open the config file: " << cnfPath << std::endl;
      return 1;
    }
    // read the whole file
    cnfFileContents = std::string(std::istreambuf_iterator<char>(cnfStream), {});
    cnfStream.close();
    cnfJSON = cJSON_Parse(cnfFileContents.c_str());
    if (cnfJSON == NULL)
    {
      const char *error_ptr = cJSON_GetErrorPtr();
      if (error_ptr != NULL)
      {
        std::cerr << "Error before: " << error_ptr << std::endl;
        return 1;
      }
    }

    config = getConfigData(cnfJSON);
    cJSON_Delete(cnfJSON);
    if (!config.validConfig)
    {
      std::cerr << config.errMsg << std::endl;
      return 1;
    }
  }
  else
  {
    //create a default config
    config.useDST = 0; // no DST
    config.startDate = -1; // use EPW start date
    config.endDate = -1; // use EPW end date
    config.firstDOY = 1; // use Jan 01 = Sunday
  }

  std::ofstream wthStream;
  wthStream.open(wthPath);
  //check that the file was opened
  if (wthStream.fail())
  {
    std::cerr << "Failed to open the wth file: " << wthPath << std::endl;
    return 1;
  }

  // check that the start date is after the end date
  if (config.endDate < config.startDate)
  {
    std::cerr << "The start date cannot be after the end date." << std::endl;
    return 1;
  }

  int retVal = convertEPW(config, epwStream, wthStream);
  epwStream.close();
  wthStream.close();
  if (retVal == 0)
    std::cout << "CONTAM Weather file created successfully." << std::endl;
  else
    std::cout << "Weather file conversion failed." << std::endl;
  return retVal;
}
#endif