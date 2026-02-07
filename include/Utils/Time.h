#pragma once

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#else
#include <chrono>
namespace chr = std::chrono;
#endif

inline auto timeNow()
{
#if defined(__EMSCRIPTEN__)
    return emscripten_get_now();
#else
    return chr::steady_clock::now();
#endif
}

using TimeType = decltype(timeNow());
//! returns time interval in milliseconds
double getDt(TimeType end, TimeType begin);
