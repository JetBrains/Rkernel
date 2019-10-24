# Rkernel
​
Rkernel is a program responsible for handling various types of requests:
code execution, code completions, debugging, and so on. Is also provides 
a response from the R interpreter.
​
The idea of Rkernel is pretty similar to 
[gdb](https://www.gnu.org/software/gdb/) or Jupyter kernel but it aimed for R.
​
## Building
​
### 1 Init git submodules
​
To build Rkernel, initialize the git submodules: `git submodules update --init`

Git submodules: 
  - [vcpkg](https://github.com/microsoft/vcpkg)
  - [Rcpp](https://github.com/RcppCore/Rcpp)
  - [Rkernel-proto](https://github.com/JetBrains/Rkernel-proto)
   ​
### 2 Build [gRPC](https://grpc.io)
   - On Windows: `build_grpc.bat`
   - On GNU/Linux or MacOS: `build_grpc.sh`
​
### 3 Use `cmake` with your toolchain to build the project
Ensure that the cmake `R_HOME` variable is defined or suitable `R` version 
is present in the `PATH` variable. 
Use the following toolchains:
​
   - On Windows: MSVS 2017 and ninja (you probably have to use `dll2lib.R` for generating `lib` files and apply patches from `Rcpp-patches`)
   - On MacOS: clang 8+ and make
   - On GNU/Linux: gcc 7.0+ and make
  
To use other configurations, you may need to edit the `CMakeLists.txt` file. 
    
## Running
Rkernel consists of:
  - Rwrapper: responsible for running gRPC server and handling interaction with R interpreter. 
  - R files: files that are located in the `R` directory of the project root.
  
For interacting with Rwrapper you should implement a gRPC client for the protocol defined in `Rkernel-proto`    
  
To start interacting with Rwrapper:
  1. Run the Rwrapper
  2. Connect using gRPC protocol
  3. Send the init message. The init messages contains your project directory and the location of R files.
  4. Once the Rwrapper receives the init message, it will source the R files.
  5. Use the message from the protocol for communication.
For more details about the protocol, see the `Rkernel-proto` project.
You could find an example of the client implementation [here](https://github.com/JetBrains/Rplugin).

## Licensing

- `Rkernel-proto` is covered by Apache License 2.0
- `R/RSession` is covered by AGPL-3.0
- the rest of the project is covered by GPL-3.0 