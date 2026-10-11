// SPDX-License-Identifier: GPL-2.0
#include "gpio_fake.hpp"

#include <asio.hpp>
#include <xapi/cmn_singleton.hpp>
#include <spdlog/spdlog.h>
#include <xapi/easylog.hpp>
#include <xapi/param_json.hpp>

#include "car_ctrl.hpp"
#include "gpio.hpp"
#include "motor.hpp"
#include "steer.hpp"

#include <dlfcn.h>
#include <sched.h>
#include <sys/mount.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

extern "C" int32_t agvctrl_main(int argc, char **argv);

namespace {

int g_failed = 0;
int g_reportFd = -1;

#define CHECK(cond)                                                       \
    do {                                                                  \
        if (!(cond)) {                                                    \
            std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            g_failed++;                                                   \
        }                                                                 \
    } while (0)

struct FakeApi {
    void (*setHostname)(const char *) {nullptr};
    void (*failNext)(int) {nullptr};
    void (*armIrq)(int) {nullptr};
    int (*ledCmds)(void) {nullptr};

    void load()
    {
        setHostname = reinterpret_cast<void (*)(const char *)>(need("test_set_hostname"));
        failNext = reinterpret_cast<void (*)(int)>(need("test_fail_next"));
        armIrq = reinterpret_cast<void (*)(int)>(need("test_arm_irq"));
        ledCmds = reinterpret_cast<int (*)(void)>(need("test_led_cmds"));
    }

    static void *need(const char *name)
    {
        void *sym = dlsym(RTLD_DEFAULT, name);
        if (!sym) {
            std::fprintf(stderr, "missing fake gpio symbol %s\n", name);
            std::exit(1);
        }
        return sym;
    }
};

FakeApi g_fake;

void clearCmd()
{
    char arg0[] = "mcarctrl_test";
    char *argv[] = {arg0};
    ParamJson::parseCmdOption(1, argv);
}

void setCmd(const char *option)
{
    char arg0[] = "mcarctrl_test";
    char arg1[] = "-c";
    char arg2[256];
    std::snprintf(arg2, sizeof(arg2), "%s", option);
    char *argv[] = {arg0, arg1, arg2};
    ParamJson::parseCmdOption(3, argv);
}

void writeFile(const std::filesystem::path &path, const std::string &text)
{
    std::ofstream ofs(path);
    ofs << text;
}

void setHost(const char *name)
{
    static bool ready = false;
    if (!ready) {
        if (unshare(CLONE_NEWNS) != 0) {
            std::perror("unshare");
            std::exit(1);
        }
        if (mount("", "/", nullptr, MS_REC | MS_PRIVATE, nullptr) != 0) {
            std::perror("mount private");
            std::exit(1);
        }
        ready = true;
    }

    std::string path = "/tmp/mcarctrl_hostname_" + std::to_string(getpid());
    writeFile(path, std::string(name) + "\n");
    if (mount(path.c_str(), "/etc/hostname", nullptr, MS_BIND, nullptr) != 0) {
        std::perror("mount hostname");
        std::exit(1);
    }
    g_fake.setHostname(name);
}

std::string pwmJson(int motors)
{
    const char *names[] = {
        "zero", "one", "two", "three", "four",
        "five", "six", "seven", "eight", "nine"
    };
    std::string out = "\"pwm\":{";
    for (int level = 0; level < 10; level++) {
        if (level)
            out += ",";
        out += "\"";
        out += names[level];
        out += "\":[";
        for (int motor = 0; motor < motors; motor++) {
            if (motor)
                out += ",";
            out += std::to_string(level * 10);
        }
        out += "]";
    }
    out += "}";
    return out;
}

class IoLoop {
public:
    IoLoop() :
        m_work(asio::make_work_guard(m_ctx)),
        m_thread([this] { m_ctx.run(); })
    {
    }

    ~IoLoop()
    {
        m_work.reset();
        m_ctx.stop();
        if (m_thread.joinable())
            m_thread.join();
    }

    asio::io_context &context() { return m_ctx; }

private:
    asio::io_context m_ctx;
    asio::executor_work_guard<asio::io_context::executor_type> m_work;
    std::thread m_thread;
};

class WorkDir {
public:
    WorkDir()
    {
        m_dir = std::filesystem::path("/tmp") / ("mcarctrl_unit_" + std::to_string(getpid()));
        std::filesystem::remove_all(m_dir);
        std::filesystem::create_directories(m_dir);
        std::filesystem::copy_file(MCARCTRL_PARAM_JSON, m_dir / "param.json");
        CHECK(chdir(m_dir.c_str()) == 0);
    }

    void chdirHere() const
    {
        CHECK(chdir(m_dir.c_str()) == 0);
    }

    const std::filesystem::path &path() const { return m_dir; }

private:
    std::filesystem::path m_dir;
};

void testGpio()
{
    {
        Gpio pin(30, GPIO_DIR_OUT, GPIO_EDGE_NONE);
        CHECK(pin.getDirection() == GPIO_DIR_OUT);
        CHECK(pin.getEdge() == GPIO_EDGE_NONE);
        CHECK(pin.getGpioFd() > 0);
        CHECK(pin.setValue(1) == 0);
        int32_t value = -1;
        CHECK(pin.getValue(&value) == 0);
        CHECK(value == 1);
        CHECK(pin.setValue(0) == 0);
        CHECK(pin.getValue(&value) == 0);
        CHECK(value == 0);
    }

    {
        Gpio first(31, GPIO_DIR_OUT, GPIO_EDGE_NONE);
        Gpio second(31, GPIO_DIR_IN, GPIO_EDGE_RISING);
        CHECK(second.getDirection() == GPIO_DIR_IN);
        CHECK(second.setValue(1) == -1);
        int32_t value = -1;
        CHECK(second.getValue(&value) == 0);
    }

    for (int edge = GPIO_EDGE_NONE; edge <= GPIO_EDGE_BOTH; edge++) {
        Gpio pin(40 + edge, GPIO_DIR_IN, edge);
        CHECK(pin.getEdge() == edge);
        CHECK(pin.getGpioFd() > 0);
    }

    g_fake.failNext(FailOpenExport);
    {
        Gpio pin(50, GPIO_DIR_OUT, GPIO_EDGE_NONE);
        CHECK(pin.getGpioFd() <= 0);
        CHECK(pin.setValue(1) == -1);
        int32_t value = 0;
        CHECK(pin.getValue(&value) == -1);
    }

    g_fake.failNext(FailWriteExport);
    {
        Gpio pin(51, GPIO_DIR_OUT, GPIO_EDGE_NONE);
        CHECK(pin.setValue(1) == -1);
    }

    g_fake.failNext(FailOpenDirection);
    {
        Gpio pin(52, GPIO_DIR_OUT, GPIO_EDGE_NONE);
    }
    g_fake.failNext(FailWriteDirection);
    {
        Gpio pin(53, GPIO_DIR_IN, GPIO_EDGE_FALLING);
    }

    g_fake.failNext(FailOpenEdge);
    {
        Gpio pin(54, GPIO_DIR_IN, GPIO_EDGE_BOTH);
    }
    g_fake.failNext(FailWriteEdge);
    {
        Gpio pin(55, GPIO_DIR_IN, GPIO_EDGE_RISING);
    }

    g_fake.failNext(FailOpenValue);
    {
        Gpio pin(56, GPIO_DIR_OUT, GPIO_EDGE_NONE);
        CHECK(pin.getGpioFd() <= 0);
    }
    g_fake.failNext(FailWriteValue);
    {
        Gpio pin(57, GPIO_DIR_OUT, GPIO_EDGE_NONE);
        CHECK(pin.setValue(1) == 0);
    }

    {
        Gpio pin(58, GPIO_DIR_OUT, GPIO_EDGE_NONE);
        g_fake.failNext(FailReadValue);
        int32_t value = 0;
        CHECK(pin.getValue(&value) == -1);
        g_fake.failNext(FailOpenUnexport);
    }

    {
        Gpio pin(59, GPIO_DIR_OUT, GPIO_EDGE_NONE);
        g_fake.failNext(FailWriteUnexport);
    }
}

void testMotor()
{
    std::vector<uint32_t> ports {61, 62};
    Motor motor(ports);
    CHECK(motor.getInputGpioFd() == 0);
    CHECK(motor.getNowState() == MotorState::Stop);
    CHECK(motor.getRunState() == MotorState::Stop);
    CHECK(motor.getMaxPwm() == 100);
    CHECK(motor.getStopPwm() == 100 - motor.getRunPwm());

    motor.setNowState(MotorState::Stop);
    motor.setNowState(MotorState::Forward);
    motor.setNowState(MotorState::Forward);
    motor.setNowState(MotorState::Back);
    motor.setNowState(MotorState::Stop);
    CHECK(motor.getNowState() == MotorState::Stop);

    motor.setCtrlSteps(8);
    CHECK(motor.getCtrlSteps() == 8);
    CHECK(motor.getRunState() == MotorState::Forward);
    motor.setCtrlSteps(0);
    CHECK(motor.getRunState() == MotorState::Stop);
    motor.setCtrlSteps(-4);
    CHECK(motor.getRunState() == MotorState::Back);

    motor.setRunState(MotorState::Forward);
    CHECK(motor.getRunState() == MotorState::Forward);
    motor.setRunPwm(30);
    CHECK(motor.getRunPwm() == 30);
    motor.setActualSteps(2);
    motor.moveActualSteps(3);
    CHECK(motor.getActualSteps() == 5);

    std::vector<uint32_t> withIr {63, 64, 65};
    Motor counted(withIr);
    CHECK(counted.getInputGpioFd() > 0);
}

void testSteer()
{
    IoLoop io;
    std::vector<uint32_t> ports {71, 72};
    Steer steer(io.context(), ports);
    steer.turn(1, 0);
    steer.turn(-1, 0);
    steer.turn(0, 0);
    steer.turn(1, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(1200));
    steer.turn(-1, 1);
    steer.turn(0, 1);
}

void exerciseCtrlSteps(CarCtrl &car, int32_t motors)
{
    CHECK(car.setCtrlSteps(motors + 1, 10) == -1);
    CHECK(car.setCtrlSteps(0, 0) == 0);
    CHECK(car.setCtrlSteps(1, 0) == 0);

    CHECK(car.setMotorSpeedLevel(9) == 0);
    CHECK(car.setCtrlSteps(1, 20) == 0);
    CHECK(car.getCtrlSteps(1) == 11);
    CHECK(car.setCtrlSteps(0, 5) == 0);
    CHECK(car.getCtrlSteps(1) == 1);
    CHECK(car.setCtrlSteps(1, -5) == 0);
    CHECK(car.getCtrlSteps(1) == -1);
    CHECK(car.setCtrlSteps(2, -20) == 0);
    CHECK(car.getCtrlSteps(2) == -11);
    CHECK(car.getCtrlSteps(motors + 3) == 0);
    CHECK(car.getActualSteps(motors + 3) == 0);
    CHECK(car.getActualSpeed(motors + 3) == 0);
    CHECK(car.getActualSpeed(1) == 0);
    CHECK(car.getCtrlMode() == CTRL_MODE_STEP);
}

void exerciseMotion(CarCtrl &car, int32_t motors)
{
    if (motors == 2) {
        CHECK(car.setCarSteps(CarDirection::dirUp, 10) == -1);
        CHECK(car.setCarMoving(CarDirection::dirRotation) == -1);
    } else {
        CHECK(car.setCarSteps(CarDirection::dirUp, 12) == 0);
        CHECK(car.setCarSteps(CarDirection::dirLeft, 6) == 0);
        CHECK(car.setCarSteps(CarDirection::dirRotation, 4) == 0);
        CHECK(car.setCarSteps(CarDirection::dirDown, 4) == 0);
        CHECK(car.setCarSteps(CarDirection::dirInvalid, 4) == 0);
    }

    CHECK(car.setCarMoving(CarDirection::dirUp) == 0);
    CHECK(car.getCtrlMode() == CTRL_MODE_TIME);
    CHECK(car.setCarMoving(CarDirection::dirDown) == 0);
    CHECK(car.setCarMoving(CarDirection::dirLeft) == 0);
    CHECK(car.setCarMoving(CarDirection::dirRight) == 0);
    if (motors != 2)
        CHECK(car.setCarMoving(CarDirection::dirRotation) == 0);
    CHECK(car.setCarMoving(CarDirection::dirInvalid) == 0);

    car.setAllMotorState(1);
    car.setAllMotorState(-1);
    car.setAllMotorState(0);
    CHECK(car.steerTurn(1, 0) == 0);
    CHECK(car.steerTurn(-1, 1) == 0);
    CHECK(car.steerTurn(0, 0) == 0);
}

void exerciseSpeedAndPwm(CarCtrl &car, int32_t motors, int32_t pwm0)
{
    CHECK(car.getMotorNum() == motors);
    CHECK(car.getMotorSpeedLevel() == 1);
    CHECK(car.getMotorPwm(1) == pwm0);
    CHECK(car.setMotorSpeedLevel(0) == -1);
    CHECK(car.setMotorSpeedLevel(10) == -1);
    CHECK(car.getMotorSpeedLevel() == 1);
    CHECK(car.setMotorSpeedLevel(9) == 0);
    CHECK(car.getMotorSpeedLevel() == 9);

    CHECK(car.setMotorSpeedLevel(1) == 0);
    car.setMotorPwm(1, 40);
    CHECK(car.getMotorPwm(1) == 40);
    car.setMotorPwm(1, 101);
    CHECK(car.getMotorPwm(1) == 40);
    car.setMotorPwm(motors + 2, 10);
    CHECK(car.getMotorPwm(motors + 2) == -1);
}

void testTwoWheel(IoLoop &io)
{
    clearCmd();
    setHost("orangepipc");
    CarCtrl car(io.context());
    exerciseSpeedAndPwm(car, 2, 10);
    exerciseCtrlSteps(car, 2);
    exerciseMotion(car, 2);

    CHECK(car.setRunTime(1) == 0);
    CHECK(car.getCtrlMode() == CTRL_MODE_TIME);
    CHECK(car.setRunTime(0) == 0);
    CHECK(car.setRunTime(-1) == 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    car.steerTurn(1, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
}

void testFourWheel(IoLoop &io)
{
    clearCmd();
    setHost("nanopim1");
    g_fake.armIrq(0);
    CarCtrl car(io.context());
    CHECK(car.getMotorNum() == 4);
    exerciseSpeedAndPwm(car, 4, 11);
    exerciseCtrlSteps(car, 4);
    exerciseMotion(car, 4);

    CHECK(car.setCtrlSteps(0, 100) == 0);
    g_fake.failNext(FailLseek);
    g_fake.armIrq(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(40));
    g_fake.failNext(FailReadValue);
    g_fake.armIrq(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(40));
    g_fake.armIrq(6);
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    CHECK(car.getActualSteps(1) > 0);
    g_fake.armIrq(0);
}

void testHostnameWithoutPi(IoLoop &io)
{
    clearCmd();
    setHost("testhost");
    CarCtrl car(io.context());
    CHECK(car.getMotorNum() == 2);
    CHECK(car.getMotorSpeedLevel() == 1);
}

void testCmdOverride(IoLoop &io)
{
    setHost("orangepipc");
    setCmd("+/orangepipc/pwm/one=[3,4]");
    CarCtrl car(io.context());
    CHECK(car.getMotorNum() == 2);
    CHECK(car.getMotorPwm(1) == 3);
    CHECK(car.getMotorPwm(2) == 4);
    clearCmd();
}

void testPartialNanopi(IoLoop &io)
{
    setHost("nanopim1");
    setCmd("+/nanopim1/motor_num=2");
    CarCtrl car(io.context());
    CHECK(car.getMotorNum() == 2);
    CHECK(car.setCarSteps(CarDirection::dirUp, 3) == -1);
    clearCmd();
}

void testJsonErrors(IoLoop &io, const WorkDir &work)
{
    clearCmd();
    setHost("orangepipc");

    auto empty = work.path() / "empty";
    std::filesystem::create_directories(empty);
    CHECK(chdir(empty.c_str()) == 0);
    {
        CarCtrl car(io.context());
        CHECK(car.getMotorNum() == 0);
    }

    work.chdirHere();
    writeFile(work.path() / "param.json", "{\"orangepipc\":{\"motor_num\":0}}");
    {
        CarCtrl car(io.context());
        CHECK(car.getMotorNum() == 0);
    }

    std::string noSteer = std::string("{\"orangepipc\":{\"motor_num\":2,")
        + "\"motor_front\":[7,8],\"motor_back\":[9,10],"
        + pwmJson(2) + "}}";
    writeFile(work.path() / "param.json", noSteer);
    {
        CarCtrl car(io.context());
        CHECK(car.getMotorNum() == 2);
        CHECK(car.steerTurn(1, 0) == 0);
    }

    std::string noNine = std::string("{\"orangepipc\":{\"motor_num\":2,")
        + "\"motor_front\":[7,8],\"motor_back\":[9,10],\"steer\":[21,20],"
        + "\"pwm\":{\"zero\":[0,0],\"one\":[10,10],\"two\":[20,20],"
        + "\"three\":[30,30],\"four\":[40,40],\"five\":[50,50],"
        + "\"six\":[60,60],\"seven\":[70,70],\"eight\":[80,80]}}}";
    writeFile(work.path() / "param.json", noNine);
    {
        CarCtrl car(io.context());
        CHECK(car.getMotorSpeedLevel() == 1);
        CHECK(car.getMotorPwm(1) == 10);
    }

    std::filesystem::copy_file(MCARCTRL_PARAM_JSON, work.path() / "param.json",
                               std::filesystem::copy_options::overwrite_existing);
}

void testRpc(IoLoop &io)
{
    clearCmd();
    setHost("nanopim1");
    CarCtrl car(io.context());
    cmn::setSingletonInstance(&car);

    CHECK(getMotorNum() == 4);
    CHECK(getMotorSpeedLevel() == 1);
    CHECK(setMotorSpeedLevel(4) == 0);
    CHECK(getMotorSpeedLevel() == 4);
    CHECK(setMotorSpeedLevel(20) == -1);
    setMotorPwm(1, 25);
    CHECK(getMotorPwm(1) == 25);
    CHECK(getMotorPwm(9) == -1);
    setAllMotorState(1);
    setAllMotorState(0);
    CHECK(setCtrlSteps(1, 15) == 0);
    CHECK(getCtrlSteps(1) > 0);
    CHECK(getActualSteps(1) >= 0);
    CHECK(getActualSpeed(1) == 0);
    CHECK(setCtrlSteps(9, 1) == -1);
    CHECK(setRunTime(1) == 0);
    CHECK(getCtrlMode() == CTRL_MODE_TIME);
    CHECK(setCarSteps(CarDirection::dirLeft, 5) == 0);
    CHECK(setCarMoving(CarDirection::dirRight) == 0);
    CHECK(setSteerTurn(1, 0) == 0);
    CHECK(setCarMoving(CarDirection::dirInvalid) == 0);

    cmn::resetSingletonInstance<CarCtrl>();
}

void testMain(const WorkDir &work)
{
    work.chdirHere();
    std::filesystem::copy_file(MCARCTRL_PARAM_JSON, work.path() / "param.json",
                               std::filesystem::copy_options::overwrite_existing);
    clearCmd();
    setHost("orangepipc");
    g_fake.armIrq(0);

    char arg0[] = "agvctrl";
    char arg1[] = "-c";
    char arg2[] = "+/orangepipc/ir_front_left=1";
    char *argv[] = {arg0, arg1, arg2};
    spdlog::drop_all();
    std::thread server([&] { agvctrl_main(3, argv); });

    for (int i = 0; i < 40 && g_fake.ledCmds() < 2; i++)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    CHECK(g_fake.ledCmds() >= 2);
    server.detach();
}

}  // namespace

extern "C" void __gcov_dump(void);

int main(int argc, char **argv)
{
    if (!std::getenv("MCARCTRL_FAKE_GPIO")) {
        if (setenv("MCARCTRL_FAKE_GPIO", "1", 1) != 0)
            return 1;
        char exe[4096];
        ssize_t len = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
        if (len <= 0)
            return 1;
        exe[len] = '\0';
        std::string lib(exe);
        auto slash = lib.rfind('/');
        lib = lib.substr(0, slash + 1) + "libmcarctrl_gpio_fake.so";
        if (setenv("LD_PRELOAD", lib.c_str(), 1) != 0)
            return 1;
        execv(exe, argv);
        std::perror("execv");
        return 1;
    }

    g_reportFd = dup(STDERR_FILENO);
    g_fake.load();
    easylog_options logOpt;
    logOpt.log_dir = "/tmp/";
    logOpt.app_log_name = "mcarctrl_test";
    logOpt.always_flush = true;
    init_log(logOpt);

    testGpio();
    testMotor();
    testSteer();

    WorkDir work;
    IoLoop io;
    testTwoWheel(io);
    testFourWheel(io);
    testHostnameWithoutPi(io);
    testCmdOverride(io);
    testPartialNanopi(io);
    testJsonErrors(io, work);
    testRpc(io);
    testMain(work);

    if (g_reportFd >= 0) {
        if (g_failed)
            dprintf(g_reportFd, "%d check(s) failed\n", g_failed);
        else
            dprintf(g_reportFd, "mcarctrl unit tests passed\n");
        close(g_reportFd);
    }

    /* The server thread is still inside agvctrl_main. Dump coverage, then
     * leave without running atexit handlers that deadlock on the logger. */
    std::fflush(stdout);
    std::fflush(stderr);
    __gcov_dump();
    std::_Exit(g_failed ? 1 : 0);
}
