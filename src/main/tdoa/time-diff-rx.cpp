#include <cpputils/Object.h>
#include <cpputils/SignalManager.h>
#include <cpputils/Timer.h>
#include <cstdio>
#include <cstring>
#include <dccomms/SerialPortStream.h>

#include <cstdio>
#include <cxxopts.hpp>
#include <sys/types.h>

using namespace std;
using namespace dccomms;
using namespace cpputils;

int main(int argc, char **argv) {
  LoggerPtr Log = cpplogging::CreateLogger("time-diff-rx");
  LoggerPtr CsvLog = cpplogging::CreateLogger("CSV");
  CsvLog->SetAsyncMode();
  std::string ac_modemPort, rf_modemPort;
  uint32_t ac_portBaudrate = 9600;
  std::string dccommsId;
  std::string logLevelStr, logFile;
  uint32_t samples = 10;
  std::string csvfile = "data.csv";
  bool flush = false, syncLog = false, hwFlowControlEnabled = false;
  Log->Info("time-diff-RX");
  try {
      cxxopts::Options options("dccomms_utils/time-diff-tx",
                               " - command line options");
      options.add_options()
      ("C,flow-control-enabled", "the flow control by hw is enabled in the modem", cxxopts::value<bool>(hwFlowControlEnabled))
      ("F,flush-log", "flush log", cxxopts::value<bool>(flush))
      ("s,sync-log", "sync-log (default: false -> async.)", cxxopts::value<bool>(syncLog))
      ("f,log-file", "File to save the log",cxxopts::value<std::string>(logFile)->default_value("")->implicit_value("bridge_log"))
      ("csv", "sample files", cxxopts::value<std::string>(csvfile))
      ("ac-modem-port", "AC Modem's serial port",cxxopts::value<std::string>(ac_modemPort)->default_value("/dev/ttyUSB0"))
      ("ac-baud-rate", "AC Serial port baudrate (default: 9600)",cxxopts::value<uint32_t>(ac_portBaudrate))
      ("samples", "default 10", cxxopts::value<uint32_t>(samples))
      ("l,log-level", "log level: critical,debug,err,info,off,trace,warn",cxxopts::value<std::string>(logLevelStr)->default_value("info"))
      ("help", "Print help");

    auto result = options.parse(argc, argv);
    if (result.count("help")) {
      std::cout << options.help({""}) << std::endl;
      exit(0);
    }

  } catch (const cxxopts::OptionException &e) {
    std::cout << "error parsing options: " << e.what() << std::endl;
    exit(1);
  }

  Log->Info("AC: port: {} ; baudrate: {} ; hw.FlowC: {}", ac_modemPort,
            ac_portBaudrate, hwFlowControlEnabled);

  auto ac_baudrate = SerialPortStream::BaudRateFromUInt(ac_portBaudrate);

  if (logFile != "") {
    Log->LogToFile(logFile);
  }
  if (flush) {
    Log->FlushLogOn(info);
    Log->Info("Flush log on info");
  }
  if (!syncLog) {
    Log->SetAsyncMode();
    Log->Info("Async. log");
  }

  auto ac0_stream = CreateObject<SerialPortStream>(ac_modemPort, ac_baudrate);

  ac0_stream->Open();
  CsvLog->LogToFile(csvfile);
  CsvLog->LogToConsole(true);
  Log->SetLogFormatter(std::make_shared<spdlog::pattern_formatter>("%T.%F %v"));
  Log->LogToConsole(true);
  CsvLog->SetLogFormatter(std::make_shared<spdlog::pattern_formatter>("%T.%F , %v"));

  ac0_stream->FlushInput();
  char ac_pre[50] = "#B0022";
  size_t ac_pre_len = std::strlen(ac_pre);
  cpputils::TimerNanos td;
  uint64_t elapsed;
  uint32_t cursample = 0;
  uint16_t count = 0;
  uint16_t rx_count = 0xffff;
  int first_its = 2;
  uint8_t count_hl[4];
  uint8_t * count_h = count_hl;
  uint8_t * count_l = count_hl+1;
  std::thread main([&]() {
    while (1) {
      ac0_stream->FlushIO();
      ac0_stream->WaitFor((const uint8_t *)ac_pre, ac_pre_len);
      int res = ac0_stream->Read(&count_hl, sizeof(count) + 2);
      rx_count = ((uint16_t)*count_h << 8) | *count_l;
      elapsed = td.Elapsed();
      if (count >= first_its) {
        cursample += 1;
        CsvLog->Info("{}", rx_count);
        if (cursample == samples) {
          Log->Info("END");
          break;
        }
      } else {
        Log->Info("SYNC: {}", rx_count);
      }
      count++;
    }
  });

  SignalManager::SetLastCallback(SIGINT, [&](int sig) {
    printf("Received %d signal\nClosing device socket...\n", sig);
    printf("Device closed.\n");
    fflush(stdout);
    Log->FlushLog();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    printf("Log messages flushed.\n");

    exit(0);
  });
  main.join();
}
