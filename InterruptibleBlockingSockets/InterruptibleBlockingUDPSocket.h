#ifndef INTERRUPTIBLE_BLOCKING_UDP_SOCKET_H
#define INTERRUPTIBLE_BLOCKING_UDP_SOCKET_H

//System includes
#ifdef _WIN32
#include <stdint.h>

#ifndef int64_t
typedef __int64 int64_t;
#endif

#ifndef uint64_t
typedef unsigned __int64 uint64_t;
#endif

#else
#include <inttypes.h>
#endif

//Library includes:
#ifndef Q_MOC_RUN //Qt's MOC and Boost have some issues don't let MOC process boost headers
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/deadline_timer.hpp>
#endif

//Local includes

class cInterruptibleBlockingUDPSocket
{

public:
    cInterruptibleBlockingUDPSocket(const std::string &strName = "");
    cInterruptibleBlockingUDPSocket(const std::string &strLocalInterface, uint16_t u16LocalPort, const std::string &strPeerAddress = "", uint16_t u16PeerPort = 60001, const std::string &strName = "");

    ~cInterruptibleBlockingUDPSocket();

    bool                            openAndBind(const std::string &strLocalAddress, uint16_t u16LocalPort);
    bool                            openBindAndConnect(const std::string &strLocalAddress, uint16_t u16LocalPort, const std::string &strPeerAddress, uint16_t u16PeerPort);
    void                            close();

    bool                            send(char *cpBuffer, uint32_t u32NBytes, uint32_t u32Timeout_ms = 0);
    bool                            sendTo(char *cpBuffer, uint32_t u32NBytes, const std::string &strPeerAddress, uint16_t u16PeerPort, uint32_t u32Timeout_ms = 0);
    bool                            sendTo(char *cpBuffer, uint32_t u32NBytes, const boost::asio::ip::udp::endpoint &oPeerEndpoint, uint32_t u32Timeout_ms = 0);

    bool                            receive(char *cpBuffer, uint32_t u32NBytes, uint32_t u32Timeout_ms = 0);
    bool                            receiveFrom(char *cpBuffer, uint32_t u32NBytes, std::string &strPeerAddress, uint16_t &u16PeerPort, uint32_t u32Timeout_ms = 0);
    bool                            receiveFrom(char *cpBuffer, uint32_t u32NBytes, boost::asio::ip::udp::endpoint &oPeerEndpoint, uint32_t u32Timeout_ms = 0);

    void                            cancelCurrrentOperations();

    //Some utility functions
    boost::asio::ip::udp::endpoint  createEndpoint(std::string strHostAddress, uint16_t u16Port);
    std::string                     getEndpointHostAddress(boost::asio::ip::udp::endpoint oEndpoint);
    uint16_t                        getEndpointPort(boost::asio::ip::udp::endpoint oEndpoint);

    //Some accessors
    boost::asio::ip::udp::endpoint  getLocalEndpoint();
    std::string                     getLocalInterface();
    uint16_t                        getLocalPort();

    boost::asio::ip::udp::endpoint  getPeerEndpoint();
    std::string                     getPeerAddress();
    uint16_t                        getPeerPort();

    std::string                     getName();

    uint32_t                        getNBytesLastTransferred();
    boost::system::error_code       getLastError();

    //Pass through some boost socket functionality:
    uint32_t                        getBytesAvailable();
    boost::asio::ip::udp::socket*   getBoostSocketPointer();

private:
    //The serial port and io service for the port
    boost::asio::io_service         m_oIOService;
    boost::asio::ip::udp::socket    m_oSocket;
    boost::asio::ip::udp::endpoint  m_oLocalEndpoint;
    boost::asio::ip::udp::endpoint  m_oPeerEndpoint;

    //Timer for deterining timeouts
    boost::asio::deadline_timer     m_oTimer;

    boost::asio::ip::udp::resolver  m_oResolver;

    //Flag for determining read errors
    bool                            m_bError;

    //Info about about last transaction
    uint32_t                        m_u32NBytesLastTransferred;
    boost::system::error_code       m_oLastError;

    //Optional label for this socket. May be useful for debugging.
    std::string                     m_strName;

    //Internal callback functions for serial port
    void                            callback_complete(const boost::system::error_code& oError, uint32_t u32NBytesTransferred);
    void                            callback_timeOut(const boost::system::error_code& oError);

};

#endif // INTERRUPTIBLE_BLOCKING_UDP_SOCKET_H
