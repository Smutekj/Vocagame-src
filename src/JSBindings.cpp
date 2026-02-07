#include "JSBindings.h"

#if defined(EMSCRIPTEN)
#include <emscripten.h>
#include <emscripten/html5.h>

EM_JS(void, setupDeviceOrientationImpl, (), {
                                                /*     if/ (typeof window.DeviceOrientationEvent != = "undefined")
                                                    {

                                                        if (typeof window.DeviceOrientationEvent.requestPermission == = "function")
                                                        {
                                                            // alert("IS FUNCTION!");
                                                            // window.DeviceOrientationEvent.requestPermission().then((response) => {
                                                            //     if (response === "granted")
                                                            //     {
                                                            //         alert("GRANTED PERMISSION!");
                                                            //         window.addEventListener("deviceorientation", (event) => {
                                                            //         Module.deviceAlpha = event.alpha;
                                                            //         Module.deviceBeta  = event.beta;
                                                            //         Module.deviceGamma = event.gamma; });
                                                            //     }
                                                            // });
                                                        }
                                                        else
                                                        {
                                                            window.addEventListener("deviceorientation", (event) = > {
                                                                Module.deviceAlpha = event.alpha;
                                                                Module.deviceBeta  = event.beta;
                                                                Module.deviceGamma = event.gamma; });
                                                        }
                                                     }*/
                                            });

EM_JS(int, isMobileImpl, (), {
    var ua = navigator.userAgent || navigator.vendor || window.opera;
    // Check for common mobile identifiers
    var isMobileDevice = /Mobi | Android | iPhone | iPad | iPod | Opera Mini | IEMobile/i.test(ua);
    isMobileDevice = isMobileDevice || window.innerWidth <= 800 || window.innerHeight <= 800;
    return isMobileDevice ? 1 : 0;
});
EM_JS(void, getStudiedLanguageImpl, (), {return Module.getStudiedLanguage()});

EM_JS(int, getWindowWidthImpl, (), { return window.screen.availWidth; });
EM_JS(int, getWindowHeightImpl, (), { return window.screen.availHeight; });

EM_JS(void, pauseGameImpl, (const char* stateName), {
    var stateNameVar = UTF8ToString(stateName);
    Module.pauseGame(stateNameVar);
});
EM_JS(void, startExamImpl, (int question_count), {
    Module.startExam(question_count);
});
EM_JS(char *, loadFromStorageImpl, (const char *key), {
    var val = localStorage.getItem(UTF8ToString(key));
    if (!val)
        return 0;
    var lengthBytes = lengthBytesUTF8(val) + 1;
    var stringOnWasmHeap = _malloc(lengthBytes);
    stringToUTF8(val, stringOnWasmHeap, lengthBytes);
    return stringOnWasmHeap;
});

double getDprImpl()
{
    return std::max(emscripten_get_device_pixel_ratio()/2., 1.);
}

EM_JS(double, getDeviceAlphaImpl, (), { return Module.deviceAlpha || 0; });
EM_JS(double, getDeviceBetaImpl, (), { return Module.deviceBeta || 0; });
EM_JS(double, getDeviceGammaImpl, (), { return Module.deviceGamma || 0; });
js::DeviceOrientation getDeviceOrientationImpl()
{
    return {getDeviceAlphaImpl(), getDeviceBetaImpl(), getDeviceGammaImpl()};
}

#else

void setupDeviceOrientationImpl() {}
void pauseGameImpl(const char* stateName) {}
void startExamImpl(int question_count) {}
bool isMobileImpl() { return false; }
char *loadFromStorageImpl(const std::string &key) { return nullptr; }
double getDprImpl() { return 1.; }

js::DeviceOrientation getDeviceOrientationImpl() { return {}; }
double getDeviceGammaImpl() { return 0.; }
double getDeviceBetaImpl() { return 0.; }
double getDeviceAlphaImpl() { return 0.; }

//! this should probably be in window?
int getWindowHeightImpl() { return 1000; }
int getWindowWidthImpl() { return 1000; }

#endif

namespace js
{

    bool is_paused = false;

    void setupDeviceOrientation() { setupDeviceOrientationImpl(); }
    bool isMobile() { return isMobileImpl(); }
    void pauseGame(std::string stateName) { is_paused = true; pauseGameImpl(stateName.c_str()); }
    void startExam(int questions_count) {is_paused = true; startExamImpl(questions_count);}
    std::string loadFromStorage(const std::string &key)
    {
        char *heap_string = loadFromStorageImpl(key.c_str());
        if (!heap_string)
        {
            return "";
        }

        std::string value = heap_string;
        free(heap_string); //! need to free string heap allocated by js, so that we don't forget it??? Is it necessary?
        return value;
    }
    double getDpr() { return getDprImpl(); }

    js::DeviceOrientation getDeviceOrientation() { return getDeviceOrientationImpl(); }
    double getDeviceGamma() { return getDeviceGammaImpl(); }
    double getDeviceBeta() { return getDeviceBetaImpl(); }
    double getDeviceAlpha() { return getDeviceAlphaImpl(); }

    int getWindowWidth() { return getWindowWidthImpl(); }
    int getWindowHeight() { return getWindowHeightImpl(); }
}