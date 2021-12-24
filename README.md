<img src="docs/fmi.svg" width="100%">

# Serverless Message Interface

## Dependencies

- C++17 or higher
- Boost
- AWS SDK for C++
- hiredis

## Installation (C++)
- Clone this repository
- Add to your CMakeLists.txt:
```cmake
add_subdirectory(path_to_repo/FMI/)

target_link_libraries(${PROJECT_NAME} PRIVATE FMI)
target_include_directories(${PROJECT_NAME} PRIVATE ${FMI_INCLUDE_DIRS})
```
- Integrate the library into your project:
```cpp
#include <Communicator.h>
...
FMI::Communicator comm(peer_id, num_peers, "config/fmi.json", "MyApp", 512);
```

## Installation (Python)
- Clone this repository
```shell
cd python
mkdir build
cd build
cmake ..
make
```
- `fmi.so` gets created in the `python/build` directory. You can copy it into your Python module path or include the build directory via `PYTHONPATH`. The library can then be integrated into your project:
```python
import fmi
comm = fmi.Communicator(peer_id, num_peers, "config/fmi.json", "MyApp", 512);
```
### Docker Container
The Docker containers [FMI-build-docker](https://github.com/OpenCoreCH/FMI-build-docker) contain all necessary dependencies and set up the environment for you. See the repo for details.

## Examples
C++ sample code for the library is available at [tests/communicator.cpp](tests/communicator.cpp), the usage from Python is demonstrated in [python/tests/client.py](python/tests/client.py). 