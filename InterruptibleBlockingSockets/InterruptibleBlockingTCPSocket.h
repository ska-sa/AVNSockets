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

#include <vector>

//Library includes:
#ifndef Q_MOC_RUN //Qt's MOC and Boost have some issues don't let MOC process boost headers
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/deadline_timer.hpp>
#endif

//Local includes

class cInterruptibleBlockingTCPSocket
{

public:
    cInterruptibleBlockingTCPSocket(const std::string &strName = "");
    cInterruptibleBlockingTCPSocket(const std::string &strPeerAddress, uint16_t u16PeerPort, const std::string &strName = "");

    ~cInterruptibleBlockingTCPSocket();

    bool                            openAndConnect(std::string strPeerAddress, uint16_t u16PeerPort, uint32_t u32Timeout_ms = 0);
    void                            close();

    bool                            send(const char *cpBuffer, uint32_t u32NBytes, uint32_t u32Timeout_ms = 0);

    bool                            receive(char *cpBuffer, uint32_t u32NBytes, uint32_t u32Timeout_ms = 0);

    bool                            readUntil(std::string &strBuffer, const std::string &strDelimiter, uint32_t u32Timeout_ms = 0);

    void                            cancelCurrrentOperations();

    //Some utility functions
    boost::asio::ip::tcp::endpoint  createEndpoint(std::string strHostAddress, uint16_t u16Port);
    std::string                     getEndpointHostAddress(boost::asio::ip::tcp::endpoint oEndPoint) const;
    uint16_t                        getEndpointPort(boost::asio::ip::tcp::endpoint oEndPoint) const;

    //Some accessors
    boost::asio::ip::tcp::endpoint  getLocalEndpoint() const;
    std::string                     getLocalInterface() const;
    uint16_t                        getLocalPort() const;

    boost::asio::ip::tcp::endpoint  getPeerEndpoint() const;
    std::string                     getPeerAddress() const;
    uint16_t                        getPeerPort() const;

    std::string                     getName() const;

    uint32_t                        getNBytesLastTransferred() const;
    boost::system::error_code       getLastError() const;

    //Pass through some boost socket functionality:
    uint32_t                        getBytesAvailable() const;
    boost::asio::ip::tcp::socket*   getBoostSocketPointer();

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

    //Internal callback functions for TCP socket port called by boost asynchronous socket API
    void                            callback_connectComplete(const boost::system::error_code& oError);
    void                            callback_connectTimeOut(const boost::system::error_code& oError);
    void                            callback_transferComplete(const boost::system::error_code& oError, uint32_t u32NBytesTransferred);
    void                            callback_transferTimeOut(const boost::system::error_code& oError);
};

#endif // INTERRUPTIBLE_BLOCKING_TCP_SOCKET_H
