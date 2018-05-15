#include <cpputils/SignalManager.h>
#include <cstdio>
#include <dccomms/CommsBridge.h>
#include <dccomms/DataLinkFrame.h>
#include <dccomms/Utils.h>
#include <dccomms_utils/S100Stream.h>
#include <iostream>

#include <cstdio>
#include <cxxopts.hpp>
#include <sys/types.h>

using namespace std;
using namespace dccomms;
using namespace dccomms_utils;
using namespace cpputils;

LoggerPtr Log = cpplogging::CreateLogger("s100bridge");
CommsBridge *bridge;
S100Stream *stream;

int main(int argc, char **argv) {
  std::string modemPort;
  uint32_t modemBitrate = 600;
  std::string dccommsId;
  std::string logLevelStr, logFile;
  bool flush = false, asyncLog = true;
  Log->Info("S100 Bridge");
  try {
    cxxopts::Options options("dccomms_utils/s100_bridge",
                             " - command line options");
    options.add_options()
        ("F,flush-log", "flush log", cxxopts::value<bool>(flush))
        ("a,async-log", "async-log", cxxopts::value<bool>(asyncLog))
        ("f,log-file", "File to save the log", cxxopts::value<std::string>(logFile)->default_value("")->implicit_value("example2_log"))
        ("p,modem-port", "Modem's serial port", cxxopts::value<std::string>(modemPort)->default_value("/dev/ttyUSB0"))
        ("l,log-level", "log level: critical,debug,err,info,off,trace,warn", cxxopts::value<std::string>(logLevelStr)->default_value("info"))
        ("b, modem-bitrate", "maximum bitrate", cxxopts::value<uint32_t>(modemBitrate))
        ("help", "Print help")
        ("dccomms-id", "dccomms id for bridge", cxxopts::value<std::string>(dccommsId)->default_value("s100"));

    auto result = options.parse(argc, argv);
    if (result.count("help")) {
      std::cout << options.help({""}) << std::endl;
      exit(0);
    }

  } catch (const cxxopts::OptionException &e) {
    std::cout << "error parsing options: " << e.what() << std::endl;
    exit(1);
  }

  Log->Info("dccommsId: {} ; port: {} ; bit-rate: {}", dccommsId, modemPort,
            modemBitrate);

  LogLevel logLevel = cpplogging::GetLevelFromString(logLevelStr);
  auto portBaudrate = SerialPortStream::BAUD_2400;
  stream = new S100Stream(modemPort, portBaudrate, modemBitrate);

  PacketBuilderPtr pb = std::make_shared<DataLinkFramePacketBuilder>(
      DataLinkFrame::fcsType::crc16);

  bridge = new CommsBridge(stream, pb, pb, 0);

  bridge->SetLogLevel(logLevel);
  bridge->SetCommsDeviceId(dccommsId);
  bridge->SetLogName("S100Bridge");
  stream->SetLogName(bridge->GetLogName() + ":S100Stream");
  stream->SetLogLevel(info);

  auto logFormatter = std::make_shared<spdlog::pattern_formatter>("[%T.%F] %v");
  stream->SetLogFormatter(logFormatter);
  bridge->SetLogFormatter(logFormatter);
  Log->SetLogFormatter(logFormatter);

  if (logFile != "") {
    Log->LogToFile(logFile);
    stream->LogToFile(logFile + "_stream");
    bridge->LogToFile(logFile + "_bridge");
  }
  if (flush) {
    Log->FlushLogOn(info);
    Log->Info("Flush log on info");
  }
  if (asyncLog) {
    Log->SetAsyncMode();
    Log->Info("Async. log");
  }

  bridge->SetReceivedPacketWithoutErrorsCb(
      [](PacketPtr pkt) { Log->Info("RX {} bytes", pkt->GetPacketSize()); });
  bridge->SetReceivedPacketWithErrorsCb(
      [](PacketPtr pkt) { Log->Warn("ERR {} bytes", pkt->GetPacketSize()); });
  bridge->SetTransmitingPacketCb(
      [](PacketPtr pkt) { Log->Info("TX {} bytes", pkt->GetPacketSize()); });

  SignalManager::SetLastCallback(SIGINT, [&](int sig) {
    printf("Received %d signal\nClosing device socket...\n", sig);
    //bridge->Stop();
    printf("Device closed.\n");
    fflush(stdout);
    bridge->FlushLog();
    stream->FlushLog();
    Log->FlushLog();
    Utils::Sleep(2000);
    printf("Log messages flushed.\n");
    exit(0);
  });

  bridge->Start();
  while (1) {
    Utils::Sleep(1000);
  }
}
