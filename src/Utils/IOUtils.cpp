#include "IOUtils.h"

#include <SDL_rwops.h>
#include <nlohmann/json.hpp>
#include "SDL_log.h"

namespace utils
{

     bool hasFileExtension(const std::string &filename, std::string extension)
    {
        int n_extension = extension.size();
        int n_filename = filename.size();
        return n_filename >= n_extension && filename.substr(n_filename - n_extension, n_filename) == extension;
    }

     std::string removeFileExtension(const std::string &filename)
    {
        auto dot_pos = filename.find_last_of('.');
        if(dot_pos == std::string::npos)
        {
            //! there was no extension
            return filename;
        }
        return filename.substr(0, dot_pos);    
    }
    
     std::vector<std::string> extractNamesInDirectory(std::filesystem::path &directory_path, std::string extension)
    {
        std::vector<std::string> names;

        for (const auto &entry : std::filesystem::directory_iterator(directory_path))
        {
            const auto filename = entry.path().filename().string();
            if (entry.is_regular_file() && hasFileExtension(filename, extension))
            {
                names.push_back(filename);
            }
        }

        return names;
    }

     nlohmann::json loadJson(const char *path)
    {
        SDL_RWops *rw = SDL_RWFromFile(path, "rb");
        if (!rw)
        {
            SDL_Log("Failed to open %s: %s", path, SDL_GetError());
            throw std::runtime_error("Could not open file");
        }

        // Read file size
        Sint64 size = SDL_RWsize(rw);
        std::vector<char> buffer(size + 1);
        SDL_RWread(rw, buffer.data(), 1, size);
        buffer[size] = '\0';
        SDL_RWclose(rw);

        return nlohmann::json::parse(buffer.begin(), buffer.end());
    }

    //! reading via SDL is a bit slower but it works even on android
     std::vector<std::byte> loadBuffer(const char *path)
    {
        SDL_RWops *rw = SDL_RWFromFile(path, "rb");
        if (!rw)
        {
            SDL_Log("Failed to open %s: %s", path, SDL_GetError());
            throw std::runtime_error("Could not open file");
        }

        // Read file size
        Sint64 size = SDL_RWsize(rw);
        std::vector<std::byte> buffer(size + 1);
        SDL_RWread(rw, buffer.data(), 1, size);
        return buffer;
    }

} // Utils