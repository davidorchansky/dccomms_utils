/*
 * main.cpp
 *
 *  Created on: 22 oct. 2016
 *      Author: Diego Centelles Beltran
 */

#include <cstdio>
#include <dccomms/Utils.h>
#include <dccomms_utils/EvologicsBridge.h>
#include <dccomms_utils/USBLStream.h>
#include <iostream>

#include <cstdio>
#include <signal.h>
#include <sys/types.h>

using namespace std;
using namespace dccomms;
using namespace dccomms_utils;

EvologicsBridge *comms;
USBLStream *stream;

void SIGINT_handler(int sig) {
  printf("Received %d signal\nClosing device socket...\n", sig);
  comms->Stop();
  printf("Device closed.\n");
  fflush(stdout);
  comms->FlushLog();
  stream->FlushLog();
  Utils::Sleep(2000);
  printf("Log messages flushed.\n");

  exit(0);
}

void setSignals() {
  if (signal(SIGINT, SIGINT_handler) == SIG_ERR) {
    printf("SIGINT install error\n");
    exit(1);
  }
}

int main(int argc, char **argv) {
  int baudrate = atoi(argv[1]);
  const char *devicedir = argv[2];

  setSignals();

  stream = new USBLStream(devicedir);
  comms = new EvologicsBridge(stream, baudrate);

  comms->SetLogLevel(cpplogging::LogLevel::debug);
  comms->SetCommsDeviceId("operator");
  comms->SetLogName("OperatorBridge");
  stream->SetLogName(comms->GetLogName() + ":USBLStream");
  comms->SetEndOfCmd("\n");

  comms->SetLocalAddr(1);
  comms->SetRemoteAddr(3);
  comms->SetClusterSize(atoi(argv[3]));

  comms->FlushLogOn(cpplogging::LogLevel::info);
  comms->LogToFile("usbl_comms_bridge_log");

  stream->FlushLogOn(cpplogging::LogLevel::info);
  stream->LogToFile("usbl_comms_bridge_device_log");

  comms->Start();
  while (1) {
    Utils::Sleep(1000);
  }
}
