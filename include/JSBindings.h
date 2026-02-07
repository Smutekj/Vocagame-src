#pragma once

#include <string>

namespace js
{
    struct DeviceOrientation
    {
        double alpha;
        double beta;
        double gamma;
    };

    void setupDeviceOrientation();
    bool isMobile();
    void pauseGame(std::string stateName);
    void startExam(int questions_count);
    std::string loadFromStorage(const std::string &key);
    double getDpr();
    //! orientation
    DeviceOrientation getDeviceOrientation();
    double getDeviceGamma();
    double getDeviceBeta();
    double getDeviceAlpha();

    
    int getWindowHeight();
    int getWindowWidth();
   
    extern bool is_paused;
    
} //! namespace js
