# Demo Framework for Multi-applications
 &emsp;The demo framework for multi-applications. All components are based on open-source software solution. Key features:

### 1) Two applications:
> The demo framework has 2 applications with different functions:
>
>   &emsp;carctrl:&emsp; control intelligent car.
>
>   &emsp;interface: user interface, such as command line(CLI) and IR remote controller.
>

### 2) RPC between applications
>   &emsp;based on yalanting/coro_rpc. Please check https://github.com/alibaba/yalantinglibs for RPC details.
>
>   &emsp;cli/src/cli_car.cpp for RPC client, mcarctrl/src/main.cpp for RPC server.
>
>   &emsp;As Interface Segregation Principle(ISP), client/server have no common header. It is import to maintain a security interface between client and server. When interface changed or not-exist, it only print error message and not coredump as general interface.
>
### 3) log shows in other tty
>   &emsp;log is based on spdlog. It is header-only library with very fast log to output/file. log shows in both tty and log-file named /var/log/appname.log, such as /var/log/carctrl.log and /var/log/interface.log.
>
>   &emsp;log file is writen with basic_file_sink_mt, not rotating_file_sink_mt. It is record log for this running. Previous running log will be covered. Please check log style in [easylog.hpp](https://github.com/whenme/carctrl/blob/main/xapi/include/xapi/easylog.hpp).
>
>   &emsp;As multi-applications run, the log cannot show in local tty. It shows in created tty. The tty name is tty_appname, such as tty_carctrl, tty_interface. It is symbolic to created tty as: tty_carctrl -> /dev/pts/2
>
>   &emsp;Please show log with tool calamares:
>
>   ./calamares -t tty_carctrl
>
### 4) run applications
>   &emsp;As multi-applications and RPC connection between applications, the applications should run at the same time. With script [execute.sh](https://github.com/whenme/carctrl/blob/main/tools/execute.sh) to run/stop applications.
>
 &emsp;The demo framework is running in OrangepiOneplus/OrangpiPc with [Armbian/Ubuntu](https://github.com/armbian/build.git). It can also run in x86 for demo.

# Build options
### 1) install required package
>   apt-get install libasio-dev nlohmann-json3-dev libspdlog-dev
>
>   apt-get install libcereal-dev libopencv-dev
>

### 2) clone code
>   git clone https://github.com/whenme/carctrl.git

### 3) build image
>   mkdir build.carctrl
>
>   cd build.carctrl
>
>   cmake ../carctrl
>
>   make
>
>   make install
>

# Run options
   &emsp;As need to create device file, it should run with root.
### 1) run all applications:
>   &emsp;./execute.sh -r

### 2) stop all applitions:
>   &emsp;./execute.sh -s

### 3) get log from tty:
>   &emsp;./calamares -t tty_carctrl
>
>   &emsp;./calamares -t tty_interface
>
> or it can show log with general commands:
>
>   &emsp;cat tty_carctrl

### 4) telnet to login CLI:
>   &emsp;telnet localhost 5000
