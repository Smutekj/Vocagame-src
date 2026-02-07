#include "Time.h"



//! returns time interval in milliseconds
double getDt(TimeType end, TimeType begin)
{
#if defined(__EMSCRIPTEN__)
    return (end - begin);
#else
    return chr::duration<double, std::milli>(end - begin).count();
#endif
}

    