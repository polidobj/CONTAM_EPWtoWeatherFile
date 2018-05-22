#include "EPWConverter.h"

#include <iostream>
#include <string>
#include <fstream>

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
    std::cout << "No command-line parameter given for epw file." << std::endl;
    return 1;
  }
  // make sure that there is at least two params
  if (argc < 3)
  {
    std::cout << "No command-line parameter given for wth file." << std::endl;
    return 1;
  }

  // assume that the first param is the epw file path
  std::string epwPath = argv[1];
  std::string wthPath = argv[2];
  bool cnfFilePresent = false;
  std::string cnfPath = "";
  std::string cnfFileContents;

  // check for a third param
  if (argc > 3)
  {
    // assume that the second param is the config file path
    cnfPath = argv[3];
    cnfFilePresent = true;
  }

  //open streams
  std::ifstream epwStream;
  epwStream.open(epwPath);
  //check that the file was opened
  if (epwStream.fail())
  {
    std::cout << "Failed to open the epw file: " << epwPath << std::endl;
    return 1;
  }

  if (cnfFilePresent)
  {
    std::ifstream cnfStream;
    cnfStream.open(cnfPath);
    //check that the file was opened
    if (cnfStream.fail())
    {
      std::cout << "Failed to open the config file: " << cnfPath << std::endl;
      return 1;
    }
    // read the whole file
    //std::string s(std::istreambuf_iterator<char>(cnfStream), {});
    //cnfFileContents = s;
    cnfFileContents = std::string(std::istreambuf_iterator<char>(cnfStream), {});
    cnfStream.close();
  }
  else
  {
    //TODO: create a default config
  }

  std::ofstream wthStream;
  wthStream.open(wthPath);
  //check that the file was opened
  if (wthStream.fail())
  {
    std::cout << "Failed to open the wth file: " << wthPath << std::endl;
    return 1;
  }

  int retVal = convertEPW(cnfFileContents, epwStream, wthStream);
  epwStream.close();
  wthStream.close();
  if (retVal == 0)
    std::cout << "CONTAM Weather file created successfully." << std::endl;
  else
    std::cout << "Weather file conversion failed." << std::endl;
  return retVal;
}