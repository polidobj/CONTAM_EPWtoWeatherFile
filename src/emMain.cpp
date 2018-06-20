#ifdef EMSCRIPTEN

#include "../lib/cJSON/cJSON.h"
#include "EPWConverter.h"
#include "config.h"
#include <emscripten.h>
#include <sstream>
#include <iostream>
#include <fstream>

extern "C" 
{
  //emscripten main loop 
  //not using this
  //but need this to keep program from exiting.
  void main_loop_iteration() 
  {
  }

  int main() 
  {
    std::cout << "Main called" << std::endl;
    //emscripten_run_script("alert('help')");
    emscripten_set_main_loop(main_loop_iteration,0 /* use browser’s framerate */,
      1 /* simulate infinite loop */);
  }

  //This function gets called to to an epw conversion
  // epwString - is a string that holds the contents of the epw file
  // configString - is a string that holds the contents of the config file (JSON)
  int ConvertEPW(char *epwPath, char *configString)
  {
    std::cout << "ConvertEPW 1.0" << std::endl;
    std::fstream epwStream;
    std::ostringstream wthStream;
    configStruct config;

    //open the epw stream
    epwStream.open(epwPath);
    //check that the file was opened
    if (epwStream.fail())
    {
      std::cerr << "Failed to open the epw file: " << epwPath << std::endl;
      return 1;
    }

    cJSON *cnfJSON;
    cnfJSON = cJSON_Parse(configString);
    if (cnfJSON == NULL)
    {
      const char *error_ptr = cJSON_GetErrorPtr();
      if (error_ptr != NULL)
      {
        std::cerr << "Error before: " << error_ptr << std::endl;
        return 1;
      }
    }
    std::cout << "Config JSON Parsed" << std::endl;

    config = getConfigData(cnfJSON);
    cJSON_Delete(cnfJSON);
    if (!config.validConfig)
    {
      std::cerr << config.errMsg << std::endl;
      return 1;
    }
    std::cout << "Config Struct completed" << std::endl;

    int retVal = convertEPW(config, epwStream, wthStream);

    //create JS code snippet
    std::string js_buffer;

    //put the simdata into a js string
    // and pass that string to a function defined in the calling space
    js_buffer = " var wthdata = `";
    js_buffer += wthStream.str();
    js_buffer += "`;passWeatherFile(wthdata);";

    // run JS snippet
    emscripten_run_script(js_buffer.c_str());
    
    return 0;
  }
}

#endif
