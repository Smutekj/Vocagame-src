#include "Application.h"

#include "JSBindings.h"

int main(int argc, char *argv[])
{
    Application app(js::getWindowWidth(), js::getWindowHeight());
    app.loadAssets();
    
    app.run();
    std::cout << "MAIN EXITED !" << std::endl;
    return 0;
}
