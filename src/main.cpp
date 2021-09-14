#include "EPWConverter.h"
#include "../lib/cJSON/cJSON.h"
#include "config.h"

#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>

#ifndef EMSCRIPTEN

// program to convert an epw file to a CONTAM weather file
// the first argument is a path to a epw file to convert
// second srgument is a path to a CONTAM weather file to create (*.wth)
// the second argument is a path to a config file
//the config file contains JSON with options for the conversion
int main(int argc, char *argv[])
{
  std::string epwPath;
  std::string wthPath;
  std::string cnfPath;
  configStruct config;

  // make sure that there is at least one param
  if (argc < 2)
  {
    std::cerr << "This tool requires at least one parameter which is the epw file path." << std::endl;
    return 1;
  }
  for (int i = 1; i < argc; ++i)
  {
    std::string argi = argv[i];
    if (argi == "--version" || argi == "-v")
    {
      printf("CONTAM_EPWtoWTH version 2.0");
      return 0;
    }
    std::filesystem::path path = argv[i];
    std::string ext = path.extension().generic_string();
    // change the extension to lowercase
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    if (ext == ".epw")
    {
      epwPath = argv[i];
    }
    else if (ext == ".cnf")
    {
      cnfPath = argv[i];
    }
    else if (ext == ".wth")
    {
      wthPath = argv[i];
    }
    else //assume it's a weather file
    {
      std::filesystem::path temp = ".wth";
      path.replace_extension(temp);
      wthPath = path.generic_string();
    }
  }

  // check if an epw path was given
  if (epwPath.empty())
  {
    // if not then can't proceed
    std::cerr << "No command-line parameter given for epw file." << std::endl;
    return 1;
  }

  // if no wth path given then use epw path
  if(wthPath.empty())
  {
    // convert the epw path to have a wth extension
    std::filesystem::path p = epwPath;
    std::filesystem::path temp = ".wth";
    p.replace_extension(temp);
    // use that path for the wth file
    wthPath = p.generic_string();
  }

  // bool to indicate if a config path was given
  bool cnfFilePresent = !cnfPath.empty();

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