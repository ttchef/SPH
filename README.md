# SPH

To build the project
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Then the build files will be in the build folder in the root directory.
If you are using Makefile on linux you can get faster build times by doing:

```bash
cmake --build build -j$(nproc)
``` 

