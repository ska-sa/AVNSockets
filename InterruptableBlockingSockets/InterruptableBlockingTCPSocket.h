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
    boost::asio::ip::tcp::endpoint  m_oBoundLocalEndPoint;
    boost::asio::ip::tcp::endpoint  m_oConnectedRemoteEndPoint;

    boost::asio::ip::tcp::resolver  m_oResolver;

    //Timer for deterining timeouts
    boost::asio::deadline_timer     m_oTimer;

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
    cInterruptibleBlockingTCPSocket(const std::string &strRemoteAddress, uint16_t u16RemotePort, const std::string &strName = "");

    void                            openAndConnectSocket(std::string strRemoteAddress, uint16_t u16RemotePort);
    void                            close();

    bool                            send(char *cpBuffer, uint32_t u32NBytes, uint32_t u32Timeout_ms = 0);

    bool                            receive(char *cpBuffer, uint32_t u32NBytes, uint32_t u32Timeout_ms = 0);

    void                            cancelCurrrentOperations();

    //Some utility functions
    boost::asio::ip::tcp::endpoint  createEndPoint(std::string strHostAddress, uint16_t u16Port);
    std::string                     getEndPointHostAddress(boost::asio::ip::tcp::endpoint oEndPoint);
    uint16_t                        getEndPointPort(boost::asio::ip::tcp::endpoint oEndPoint);

    //Some accessors
    boost::asio::ip::tcp::endpoint  getBoundLocalEndpoint();
    std::string                     getBoundLocalAddress();
    uint16_t                        getBoundLocalPort();

    boost::asio::ip::tcp::endpoint  getConnectedRemoteEndPoint();
    std::string                     getConnectedRemoteAddress();
    uint16_t                        getConnectedRemotePort();

    std::string                     getName();

    uint32_t                        getNBytesLastTransferred();
    boost::system::error_code       getLastError();

    //Pass through some boost socket functionality:
    uint32_t                        getBytesAvailable();
    boost::asio::ip::tcp::socket*   getBoostSocketPointer();

};

#endif // INTERRUPTIBLE_BLOCKING_TCP_SOCKET_H
