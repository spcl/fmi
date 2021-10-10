# SMI - Serverless Message Interface

## Dependencies

- C++14 or higher
- Boost
- AWS SDK for C++
- hiredis

## Installation (C++)
- Clone this repository
- Add to your CMakeLists.txt:
```cmake
add_subdirectory(path_to_repo/SMI/)

target_link_libraries(${PROJECT_NAME} PRIVATE SMI)
target_include_directories(${PROJECT_NAME} PRIVATE ${SMI_INCLUDE_DIRS})
```
- Integrate the library into your project:
```cpp
#include <Communicator.h>
...
SMI::Communicator comm(peer_id, num_peers, "config/smi.json", "MyApp", 512);
```