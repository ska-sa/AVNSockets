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

    //Do not guarantee all bytes sent
    bool                            send(const char *cpBuffer, uint32_t u32NBytes, uint32_t u32Timeout_ms = 0);
    bool                            receive(char *cpBuffer, uint32_t u32NBytes, uint32_t u32Timeout_ms = 0);

    //Guarantee all bytes sent
    bool                            write(const char *cpBuffer, uint32_t u32NBytes, uint32_t u32Timeout_ms = 0);
    bool                            write(const std::string &strData, uint32_t u32Timeout_ms = 0); //Convenience function for sending of text
    bool                            read(char *cpBuffer, uint32_t u32NBytes, uint32_t u32Timeout_ms = 0);
    bool                            readUntil(std::string &strBuffer, const std::string &strDelimiter, uint32_t u32Timeout_ms = 0); //Convenience function, read until a delimeter is found.

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

    uint32_t                        getNBytesLastRead() const;
    uint32_t                        getNBytesLastWritten() const;
    boost::system::error_code       getLastReadError() const;
    boost::system::error_code       getLastWriteError() const;
    boost::system::error_code       getLastOpenAndConnectError() const;

    //Pass through some boost socket functionality:
    uint32_t                        getBytesAvailable() const;
    boost::asio::ip::tcp::socket*   getBoostSocketPointer();

private:
    //The serial port and io service for the port
    boost::asio::io_service         m_oIOService;
    boost::asio::ip::tcp::socket    m_oSocket;

    //Timer for determining timeouts
    boost::asio::deadline_timer     m_oOpenAndConnectTimer;
    boost::asio::deadline_timer     m_oReadTimer;
    boost::asio::deadline_timer     m_oWriteTimer;

    boost::asio::ip::tcp::resolver  m_oResolver;

    //Flag for determining read errors
    bool                            m_bOpenAndConnectError;
    bool                            m_bReadError;
    bool                            m_bWriteError;

    //Info about about last transaction
    uint32_t                        m_u32NBytesLastRead;
    uint32_t                        m_u32NBytesLastWritten;
    boost::system::error_code       m_oLastReadError;
    boost::system::error_code       m_oLastWriteError;
    boost::system::error_code       m_oLastopenAndConnectError;

    //Receive string used by readUntil function
    //(Require persistence across calls)
    std::string                      m_strReadUntilBuff;

    //Optional label for this socket. May be useful for debugging.
    std::string                     m_strName;

    //Internal callback functions for TCP socket port called by boost asynchronous socket API
    void                            callback_connectComplete(const boost::system::error_code& oError);
    void                            callback_connectTimeOut(const boost::system::error_code& oError);
    void                            callback_readComplete(const boost::system::error_code& oError, uint32_t u32NBytesTransferred);
    void                            callback_readTimeOut(const boost::system::error_code& oError);
    void                            callback_writeComplete(const boost::system::error_code& oError, uint32_t u32NBytesTransferred);
    void                            callback_writeTimeOut(const boost::system::error_code& oError);
};

#endif // INTERRUPTIBLE_BLOCKING_TCP_SOCKET_H
