#ifndef INTERRUPTIBLE_BLOCKING_TCP_SOCKET_H
#define INTERRUPTIBLE_BLOCKING_TCP_SOCKET_H

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
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/deadline_timer.hpp>
#endif

//Local includes

class cInterruptibleBlockingTCPSocket
{
private:
    //The serial port and io service for the port
    boost::asio::io_service         m_oIOService;
    boost::asio::ip::tcp::socket    m_oSocket;

    //Timer for determining timeouts
    boost::asio::deadline_timer     m_oTimer;

    boost::asio::ip::tcp::resolver  m_oResolver;

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

public:
    cInterruptibleBlockingTCPSocket(const std::string &strName = "");
    cInterruptibleBlockingTCPSocket(const std::string &strPeerAddress, uint16_t u16PeerPort, const std::string &strName = "");

    bool                            openAndConnect(std::string strPeerAddress, uint16_t u16PeerPort);
    void                            close();

    bool                            send(char *cpBuffer, uint32_t u32NBytes, uint32_t u32Timeout_ms = 0);

    bool                            receive(char *cpBuffer, uint32_t u32NBytes, uint32_t u32Timeout_ms = 0);

    void                            cancelCurrrentOperations();

    //Some utility functions
    boost::asio::ip::tcp::endpoint  createEndpoint(std::string strHostAddress, uint16_t u16Port);
    std::string                     getEndpointHostAddress(boost::asio::ip::tcp::endpoint oEndPoint);
    uint16_t                        getEndpointPort(boost::asio::ip::tcp::endpoint oEndPoint);

    //Some accessors
    boost::asio::ip::tcp::endpoint  getLocalEndpoint();
    std::string                     getLocalInterface();
    uint16_t                        getLocalPort();

    boost::asio::ip::tcp::endpoint  getPeerEndpoint();
    std::string                     getPeerAddress();
    uint16_t                        getPeerPort();

    std::string                     getName();

    uint32_t                        getNBytesLastTransferred();
    boost::system::error_code       getLastError();

    //Pass through some boost socket functionality:
    uint32_t                        getBytesAvailable();
    boost::asio::ip::tcp::socket*   getBoostSocketPointer();

};

#endif // INTERRUPTIBLE_BLOCKING_TCP_SOCKET_H
