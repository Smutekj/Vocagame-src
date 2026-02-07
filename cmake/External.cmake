include(FetchContent)
  
## RENDERER
#FetchContent_Declare(
#  renderer  
#  GIT_REPOSITORY https://github.com/Smutekj/simple-emscripten-renderer
#  GIT_TAG master
#)
#FetchContent_MakeAvailable(renderer)

### CONSTRAINED DELAUNAY TRIANGULATION
FetchContent_Declare(
  CDT
  GIT_REPOSITORY https://github.com/Smutekj/CDT
  GIT_TAG main 
  )
FetchContent_MakeAvailable(CDT)
  


FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
FetchContent_MakeAvailable(json)

