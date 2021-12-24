<img src="docs/fmi.svg" width="100%">

# FaaS Message Interface

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
### Docker Images
The Docker images [FMI-build-docker](https://github.com/OpenCoreCH/FMI-build-docker) contain all necessary dependencies and set up the environment for you. See the repo for details.

### AWS Lambda Layer
For even easier deployment, we provide AWS CloudFormation templates to create Lambda layers in [python/aws](python/aws). Simply run `sam build` and `sam deploy --guided` in the folder corresponding to your Python version, which creates a Lambda layer in your account that can be added to your function. As soon as you added the layer, you can simply use `import fmi` and work with the library.

## Examples
C++ sample code for the library is available at [tests/communicator.cpp](tests/communicator.cpp), the usage from Python is demonstrated in [python/tests/client.py](python/tests/client.py). 