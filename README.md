# Demo Framework for Multi-applications
> The demo framework for multi-applications features:
>
### 1) Two applications:
> The demo framework has 2 applications with different functions:
>    > carctrl:   control intelligent car
>    > interface: user interface, such as command line(CLI) and IR remote controller
>
### 2) RPC between applications
>   based on asio2. Please check https://github.com/zhllxt/asio2.git for RPC details.
>
>   cli/src/cli_car.cpp for RPC client, mcarctrl/src/main.cpp for RPC server
>
>   As Interface Segregation Principle(ISP), client/server have no common header. It is import to maintain a security interface between client and server. When interface changed or not-exist, it only print error message and not coredump as general interface.
>
### 3) log shows in other tty
>   log is based on spdlog. It is header-only library with very fast log output/file. log shows in both tty and log file named /var/log/appname.log, such as /var/log/carctrl.log and /var/log/interface.log.
>   log file is writen with basic_file_sink_mt, not rotating_file_sink_mt. It is record log of this running. Previous running log will be covered. Please check log style in ioapi/include/ioapi/easylog.hpp.
>   As multi-applications run, the log cannot show in local tty. It shows in created tty. The tty name is tty_appname, such as tty_carctrl, tty_interface. It is symbolic to created tty as: tty_carctrl -> /dev/pts/2
>
>   Please show log with tool calamares:
>   ./calamares -t tty_carctrl
>
### 4) run applications
>   As multi-applications and RPC connection between applications, the applications should run at the same time. With execute.sh to run/stop applications.
>
> The demo framework is running in OrangepiOneplus/OrangpiPc with [Armbian/Ubuntu](https://github.com/armbian/build.git). It can also run in x86 for demo.
>

# Build options
### 1) install required package
>   apt-get install libasio-dev nlohmann-json3-dev libspdlog-dev

### 2) clone code
>   git clone https://github.com/whenme/carctrl.git

### 3) build image
>   mkdir build.carctrl
>   cd build.carctrl
>   cmake ../carctrl
>   make
>   make install

# Run options
   As need to create device file, it should run with root.
### 1) run all applications:
>   ./execute.sh -r
>

### 2) stop all applitions:
>   ./execute.sh -s

### 3) get log from tty:
>   ./calamares -t tty_carctrl
>   ./calamares -t tty_interface
>
> or it can show log with general commands:
>   cat tty_carctrl

### 4) telnet to login CLI:
>   telnet localhost 5000
