/*
 * EvologicsBridge.h
 *
 *  Created on: 18 nov. 2016
 *      Author: centelld
 */

#ifndef INCLUDE_MERBOTS_EVOLOGICSBRIDGE_H_
#define INCLUDE_MERBOTS_EVOLOGICSBRIDGE_H_

#include <dccomms/CommsBridge.h>
#include <mutex>
namespace dccomms_utils {

using namespace dccomms;

class EvologicsBridge: public CommsBridge{
public:
        EvologicsBridge(ICommsDevice *, int _baudrate = 0, DataLinkFrame::fcsType _chksum = DataLinkFrame::fcsType::crc32);
	virtual ~EvologicsBridge();

	void SetRemoteAddr(int);
	void SetLocalAddr(int);
	void SetEndOfCmd(std::string);
	void SetClusterSize(int);
private:
        std::mutex lockTransmission;
	int remoteAddr, localAddr,
		clusterSize;
	void TxWork();
	void ClearTransmissionBuffer(); //Send ATZ4
	bool TryToConnect();
	void _InitCommands();
	void _SendInitCommands();
	std::string endOfCmd,   //End of command
				ATZ4,       //Clear transmission buffer
				AT0,		//Ensure data mode
				ATRP0,		//Disable promiscous mode
				ATZU0,		//Disable USBLLONG (will take effect only in USBL modem
				ATZX0,		//Disable extended notifications
				ATZC,		//Set cluster size 30 (30 packets per packet train)
				ATALX,		//Set local address
				ATARX,		//Set remote address
				ATKO0,		//Keep online the acoustic connection
				ATD; 		//Establish a connection
};

} /* namespace merbots */

#endif /* INCLUDE_MERBOTS_EVOLOGICSBRIDGE_H_ */
