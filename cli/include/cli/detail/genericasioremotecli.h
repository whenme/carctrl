// SPDX-License-Identifier: GPL-2.0

#ifndef __CLI_DETAIL_GENERICASIOREMOTECLI_H_INCLUDED__
#define __CLI_DETAIL_GENERICASIOREMOTECLI_H_INCLUDED__

#include <memory>

#include "../cli.h"
#include "genericasioscheduler.h"
#include "inputdevice.h"
#include "inputhandler.h"
#include "server.h"

namespace cli::detail
{
class TelnetSession : public Session
{
public:
    explicit TelnetSession(asiolib::ip::tcp::socket sock) : Session(std::move(sock))
    {
    }

protected:
    std::string encode(const std::string& dat) const override
    {
        std::string result;
        for (const char& ch : dat)
        {
            if (ch == '\n')
            {
                result += '\r';
            }
            result += ch;
        }
        return result;
    }

    void onConnect() override
    {
        // to specify hexadecimal value as chars we use
        // the syntax \xVVV
        // and the std::string ctor that takes the size,
        // so that it's not null-terminated
        // std::string msg{ "\x0FF\x0FD\x027", 3 };
        // waitAck = true;
        // std::string iacDoSuppressGoAhead{ "\x0FF\x0FD\x003", 3 };
        // this -> OutStream() << iacDoSuppressGoAhead << std::flush;

        // https://www.ibm.com/support/knowledgecenter/SSLTBW_1.13.0/com.ibm.zos.r13.hald001/telcmds.htm

        static const std::string k_iacDoLineMode{"\x0FF\x0FD\x022", 3};
        this->outStream() << k_iacDoLineMode << std::flush;

        static const std::string k_iacSbLineMode0IacSe{"\x0FF\x0FA\x022\x001\x000\x0FF\x0F0", 7};
        this->outStream() << k_iacSbLineMode0IacSe << std::flush;

        static const std::string k_iacWillEcho{"\x0FF\x0FB\x001", 3};
        this->outStream() << k_iacWillEcho << std::flush;
    }
    void onDisconnect() override
    {
    }
    void onError() override
    {
    }

#if 0
    void onDataReceived(const std::string& data) override
    {
        if (waitAck)
        {
            if ( data[0] == '\x0FF' )
            {
                // TODO
                for (size_t i = 0; i < data.size(); ++i)
                    std::cout << static_cast<int>( data[i] & 0xFF ) << ' ';
                std::cout << std::endl;

                for (size_t i = 0; i < data.size(); ++i)
                    std::cout << "0x" << std::hex << static_cast<int>( data[i] & 0xFF ) << std::dec << ' ';
                std::cout << std::endl;

                for (size_t i = 0; i < data.size(); ++i)
                    switch (static_cast<int>( data[i] & 0xFF ))
                    {
                        case 0xFF: std::cout << "IAC "; break;
                        case 0xFE: std::cout << "DONT "; break;
                        case 0xFD: std::cout << "DO "; break;
                        case 0xFC: std::cout << "WONT "; break;
                        case 0xFB: std::cout << "WILL "; break;
                        case 0xFA: std::cout << "SB "; break;
                        case 0xF9: std::cout << "GoAhead "; break;
                        case 0xF8: std::cout << "EraseLine "; break;
                        case 0xF7: std::cout << "EraseCharacter "; break;
                        case 0xF6: std::cout << "AreYouThere "; break;
                        case 0xF5: std::cout << "AbortOutput "; break;
                        case 0xF4: std::cout << "InterruptProcess "; break;
                        case 0xF3: std::cout << "Break "; break;
                        case 0xF2: std::cout << "DataMark "; break;
                        case 0xF1: std::cout << "NOP "; break;
                        case 0xF0: std::cout << "SE "; break;
                        default: std::cout << (static_cast<int>( data[i] & 0xFF )) << ' ';
                    }
                std::cout << std::endl;
            }
            waitAck = false;
        }
        else
        {
            for (size_t i = 0; i < data.size(); ++i)
                feed(data[i]);
        }
    }
#else
    /*
    See
    https://www.iana.org/assignments/telnet-options/telnet-options.xhtml
    for a list of telnet options
    */
    enum
    {
        SE       = '\x0F0',          // End of subnegotiation parameters.
        NOP      = '\x0F1',          // No operation.
        DataMark = '\x0F2',          // The data stream portion of a Synch.
                                     // This should always be accompanied
                                     // by a TCP Urgent notification.
        Break            = '\x0F3',  // NVT character BRK.
        InterruptProcess = '\x0F4',  // The function IP.
        AbortOutput      = '\x0F5',  // The function AO.
        AreYouThere      = '\x0F6',  // The function AYT.
        EraseCharacter   = '\x0F7',  // The function EC.
        EraseLine        = '\x0F8',  // The function EL.
        GoAhead          = '\x0F9',  // The GA signal.
        SB               = '\x0FA',  // Indicates that what follows is
                                     // subnegotiation of the indicated
                                     // option.
        WILL = '\x0FB',              // Indicates the desire to begin
                                     // performing, or confirmation that
                                     // you are now performing, the
                                     // indicated option.
        WONT = '\x0FC',              // Indicates the refusal to perform,
                                     // or continue performing, the
                                     // indicated option.
        DO = '\x0FD',                // Indicates the request that the
                                     // other party perform, or
                                     // confirmation that you are expecting
                                     // the other party to perform, the
                                     // indicated option.
        DONT = '\x0FE',              // Indicates the demand that the
                                     // other party stop performing,
                                     // or confirmation that you are no
                                     // longer expecting the other party
                                     // to perform, the indicated option.
        IAC = '\x0FF',               // Data Byte 255.

        ECHOO                 = '\x001',
        SuppressGoAhead       = '\x003',
        TerminalType          = '\x018',
        NegotiateAboutWinSize = '\x01F',
        TermainalSpeed        = '\x020',
        NewEnvOption          = '\x027'
    };

    void onDataReceived(const std::string& dat) override
    {
        for (const auto& ch : dat)
        {
            consume(ch);
        }
    }

private:
    void consume(char ch)
    {
        if (m_escape)
        {
            if (ch == IAC)
            {
                data(ch);
            }
            else
            {
                command(ch);
            }
            m_escape = false;
        }
        else
        {
            if (ch == IAC)
            {
                m_escape = true;
            }
            else
            {
                data(ch);
            }
        }
    }

    void data(char ch)
    {
        switch (m_state)
        {
            case State::Data:
                output(ch);
                break;
            case State::Sub:
                rxSub(ch);
                break;
            case State::WaitWill:
                rxWill(ch);
                m_state = State::Data;
                break;
            case State::WaitWont:
                rxWont(ch);
                m_state = State::Data;
                break;
            case State::WaitDo:
                rxDo(ch);
                m_state = State::Data;
                break;
            case State::WaitDont:
                rxDont(ch);
                m_state = State::Data;
                break;
            default:
                break;
        }
    }

    void command(char ch)
    {
        switch (ch)
        {
            case SE:
                if (m_state == State::Sub)
                {
                    m_state = State::Data;
                }
                else
                {
                    std::cout << "ERROR: received SE when not in sub state" << std::endl;
                }
                break;
            case DataMark:
            case Break:
            case InterruptProcess:
            case AbortOutput:
            case AreYouThere:
            case EraseCharacter:
            case EraseLine:
            case GoAhead:
            case NOP:
                m_state = State::Data;
                break;
            case SB:
                if (m_state != State::Sub)
                {
                    m_state = State::Sub;
                }
                else
                {
                    std::cout << "ERROR: received SB when already in sub state" << std::endl;
                }
                break;
            case WILL:
                m_state = State::WaitWill;
                break;
            case WONT:
                m_state = State::WaitWont;
                break;
            case DO:
                m_state = State::WaitDo;
                break;
            case DONT:
                m_state = State::WaitDont;
                break;
            case IAC:
                assert(false);  // can't be here
                m_state = State::Data;
                break;
            default:
                break;
        }
    }

    void rxWill(char ch)
    {
#ifdef CLI_TELNET_TRACE
        std::cout << "will " << static_cast<int>(ch) << std::endl;
#endif
        switch (ch)
        {
            case SuppressGoAhead:
                sendIacCmd(WILL, SuppressGoAhead);
                break;
            case NegotiateAboutWinSize:
                sendIacCmd(DO, NegotiateAboutWinSize);
                break;
            default:
                sendIacCmd(DONT, ch);
        }
    }
    static void rxWont(char ch)
    {
#ifdef CLI_TELNET_TRACE
        std::cout << "wont " << static_cast<int>(ch) << std::endl;
#else
        (void)ch;
#endif
    }
    void rxDo(char ch)
    {
#ifdef CLI_TELNET_TRACE
        std::cout << "do " << static_cast<int>(ch) << std::endl;
#endif
        switch (ch)
        {
            case ECHOO:
                sendIacCmd(DO, ECHOO);
                break;
            case SuppressGoAhead:
                sendIacCmd(WILL, SuppressGoAhead);
                break;
            default:
                sendIacCmd(WONT, ch);
        }
    }
    static void rxDont(char ch)
    {
#ifdef CLI_TELNET_TRACE
        std::cout << "dont " << static_cast<int>(ch) << std::endl;
#else
        (void)ch;
#endif
    }
    static void rxSub(char ch)
    {
#ifdef CLI_TELNET_TRACE
        std::cout << "sub: " << static_cast<int>(ch) << std::endl;
#else
        (void)ch;
#endif
    }
    void sendIacCmd(char action, char op)
    {
        std::string answer("\x0FF\x000\x000", 3);
        answer[1] = action;
        answer[2] = op;
        this->outStream() << answer << std::flush;
    }

protected:
    virtual void output(signed char ch)
    {
#ifdef CLI_TELNET_TRACE
        std::cout << "data: " << static_cast<int>(ch) << std::endl;
#else
        (void)ch;
#endif
    }

private:
    enum class State
    {
        Data,
        Sub,
        WaitWill,
        WaitWont,
        WaitDo,
        WaitDont
    };
    State m_state  = State::Data;
    bool  m_escape = false;
#endif

    static void feed(char ch)
    {
        if (std::isprint(ch) != 0)
        {
            std::cout << ch << std::endl;
        }
        else
        {
            std::cout << "0x" << std::hex << static_cast<int>(ch) << std::dec << std::endl;
        }
    }
    std::string m_buffer;
};

template<typename ASIOLIB>
class TelnetServer : public Server<ASIOLIB>
{
public:
    TelnetServer(typename ASIOLIB::ContextType& ios, unsigned short port) : Server<ASIOLIB>(ios, port)
    {
    }
    std::shared_ptr<Session> createSession(asiolib::ip::tcp::socket sock) override
    {
        return std::make_shared<TelnetSession>(std::move(sock));
    }
};

class CliTelnetSession : public InputDevice, public TelnetSession, public CliSession
{
public:
    CliTelnetSession(Scheduler&                                sched,
                     asiolib::ip::tcp::socket                  sock,
                     Cli&                                      cli,
                     const std::function<void(std::ostream&)>& exitAct,
                     std::size_t                               historySize) :
        InputDevice(sched),
        TelnetSession(std::move(sock)),
        CliSession(cli, TelnetSession::outStream(), historySize),
        m_poll(*this, *this)
    {
        exitAction([this, exitAct](std::ostream& out) {
            exitAct(out);
            disConnect();
        });
    }

protected:
    void onConnect() override
    {
        TelnetSession::onConnect();
        prompt();
    }

    void output(signed char ch) override  // NB: C++ does not specify wether char is signed or unsigned
    {
        switch (m_step)
        {
            case Step::Step1:
                switch (ch)
                {
                    case EOF:
                    case k_KeyEot:  // EOT
                        notify(std::make_pair(KeyType::Eof, ' '));
                        break;
                    case k_KeyBack:       // Backspace
                    case k_KeyBackspace:  // Backspace or Delete
                        notify(std::make_pair(KeyType::Backspace, ' '));
                        break;
                    // case 10: Notify(std::make_pair(KeyType::ret,' ')); break;
                    case k_KeySymbol:  // symbol
                        m_step = Step::Step2;
                        break;
                    case k_KeyWait0:  // wait for 0 (ENTER key)
                        m_step = Step::Wait0;
                        break;
                    default:  // ascii
                    {
                        const char chr = static_cast<char>(ch);
                        notify(std::make_pair(KeyType::Ascii, chr));
                    }
                }
                break;

            case Step::Step2:          // got 27 last time
                if (ch == k_KeyArrow)  // arrow keys
                {
                    m_step = Step::Step3;
                }
                else  // unknown
                {
                    m_step = Step::Step1;
                    notify(std::make_pair(KeyType::Ignored, ' '));
                }
                break;

            case Step::Step3:  // got 27 and 91
                switch (ch)
                {
                    case k_KeyUp:
                        m_step = Step::Step1;
                        notify(std::make_pair(KeyType::Up, ' '));
                        break;
                    case k_KeyDown:
                        m_step = Step::Step1;
                        notify(std::make_pair(KeyType::Down, ' '));
                        break;
                    case k_KeyLeft:
                        m_step = Step::Step1;
                        notify(std::make_pair(KeyType::Left, ' '));
                        break;
                    case k_KeyRight:
                        m_step = Step::Step1;
                        notify(std::make_pair(KeyType::Right, ' '));
                        break;
                    case k_KeyEnd:
                        m_step = Step::Step1;
                        notify(std::make_pair(KeyType::End, ' '));
                        break;
                    case k_KeyHome:
                        m_step = Step::Step1;
                        notify(std::make_pair(KeyType::Home, ' '));
                        break;
                    default:
                        m_step = Step::Step4;
                        break;  // not arrow keys
                }
                break;

            case Step::Step4:
                if (ch == k_KeyCanc)
                {
                    notify(std::make_pair(KeyType::Canc, ' '));
                }
                else
                {
                    notify(std::make_pair(KeyType::Ignored, ' '));
                }
                m_step = Step::Step1;
                break;

            case Step::Wait0:
                if (ch == 0 /* linux */ || ch == k_KeyRet /* win */)
                {
                    notify(std::make_pair(KeyType::Ret, ' '));
                }
                else
                {
                    notify(std::make_pair(KeyType::Ignored, ' '));
                }
                m_step = Step::Step1;
                break;
        }
    }

private:
    enum class Step
    {
        Step1,
        Step2,
        Step3,
        Step4,
        Wait0
    };
    Step         m_step = Step::Step1;
    InputHandler m_poll;

    static constexpr int32_t k_KeyEot       = 4;
    static constexpr int32_t k_KeyBack      = 8;
    static constexpr int32_t k_KeyBackspace = 127;
    static constexpr int32_t k_KeyRet       = 10;
    static constexpr int32_t k_KeySymbol    = 27;
    static constexpr int32_t k_KeyArrow     = 91;
    static constexpr int32_t k_KeyCanc      = 126;
    static constexpr int32_t k_KeyUp        = 65;
    static constexpr int32_t k_KeyDown      = 66;
    static constexpr int32_t k_KeyLeft      = 68;
    static constexpr int32_t k_KeyRight     = 67;
    static constexpr int32_t k_KeyHome      = 72;
    static constexpr int32_t k_KeyEnd       = 70;
    static constexpr int32_t k_KeyWait      = 51;
    static constexpr int32_t k_KeyWait0     = 13;
};

template<typename ASIOLIB>
class CliGenericTelnetServer : public Server<ASIOLIB>
{
public:
    CliGenericTelnetServer(Cli&                           cli,
                           GenericAsioScheduler<ASIOLIB>& sched,
                           unsigned short                 port,
                           std::size_t                    historySize = k_MHistorySizeDef) :
        Server<ASIOLIB>(sched.getAsioContext(), port),
        m_scheduler(sched),
        m_cli(cli),
        m_historySize(historySize)
    {
    }
    CliGenericTelnetServer(Cli&                           cli,
                           GenericAsioScheduler<ASIOLIB>& sched,
                           std::string                    address,
                           unsigned short                 port,
                           std::size_t                    historySize = k_MHistorySizeDef) :
        Server<ASIOLIB>(sched.getAsioContext(), address, port),
        m_scheduler(sched),
        m_cli(cli),
        m_historySize(historySize)
    {
    }
    void exitAction(std::function<void(std::ostream&)> action)
    {
        m_exitAction = action;
    }
    std::shared_ptr<Session> createSession(asiolib::ip::tcp::socket sock) override
    {
        return std::make_shared<CliTelnetSession>(m_scheduler, std::move(sock), m_cli, m_exitAction, m_historySize);
    }

private:
    Scheduler&                         m_scheduler;
    Cli&                               m_cli;
    std::function<void(std::ostream&)> m_exitAction;
    std::size_t                        m_historySize;

    static constexpr std::size_t k_MHistorySizeDef = 100;
};

}  // namespace cli::detail

#endif  // __CLI_DETAIL_GENERICASIOREMOTECLI_H_INCLUDED__
