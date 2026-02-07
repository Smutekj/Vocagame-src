#pragma once

#include <string>
#include <vector>
#include <filesystem>

#include <nlohmann/json.hpp>


namespace utils
{

     bool hasFileExtension(const std::string &filename, std::string extension);

     std::string removeFileExtension(const std::string &filename);
    
     std::vector<std::string> extractNamesInDirectory(std::filesystem::path &directory_path, std::string extension);
    
     nlohmann::json loadJson(const char *path);

    //! reading via SDL is a bit slower but it works even on android
     std::vector<std::byte> loadBuffer(const char *path);
    

} // Utils