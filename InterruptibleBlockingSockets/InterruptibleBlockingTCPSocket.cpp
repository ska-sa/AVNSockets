
//System includes
#include <iostream>

//Library includes
#ifndef Q_MOC_RUN //Qt's MOC and Boost have some issues don't let MOC process boost headers
#include <boost/bind.hpp>
#include <boost/asio/placeholders.hpp>
#endif

//Local includes
#include "InterruptibleBlockingTCPSocket.h"

using namespace std;

cInterruptibleBlockingTCPSocket::cInterruptibleBlockingTCPSocket(const string &strName) :
    m_oSocket(m_oIOService),
    m_oTimer(m_oIOService),
    m_oResolver(m_oIOService),
    m_bError(true),
    m_strName(strName)
{
}

cInterruptibleBlockingTCPSocket::cInterruptibleBlockingTCPSocket(const string &strRemoteAddress, uint16_t u16RemotePort, const string &strName) :
    m_oSocket(m_oIOService),
    m_oTimer(m_oIOService),
    m_oResolver(m_oIOService),
    m_bError(true),
    m_strName(strName)
{
    openAndConnect(strRemoteAddress, u16RemotePort);
}

bool cInterruptibleBlockingTCPSocket::openAndConnect(string strPeerAddress, uint16_t u16PeerPort, uint32_t u32Timeout_ms)
{
    //Necessary after a timeout:
    m_oSocket.get_io_service().reset();

    //If the socket is already open close it
    close();

    //Open the socket
    m_oSocket.open(boost::asio::ip::tcp::v4(), m_oLastError);

    if(m_oLastError)
    {
        cout << "cInterruptibleBlockingTCPSocket::openAndConnect(): Error opening socket: " << m_oLastError.message() << endl;
        return false;
    }
    else
    {
        cout << "cInterruptibleBlockingTCPSocket::openAndConnect(): Successfully opened TCP socket. Attempting to connect..." << endl;
    }
    fflush(stdout);

    //Set some socket options
    m_oSocket.set_option( boost::asio::socket_base::receive_buffer_size(64 * 1024 * 1024) ); //Set buffer to 64 MB
    m_oSocket.set_option( boost::asio::socket_base::reuse_address(true) );

    boost::asio::ip::tcp::endpoint oPeerEndPoint = createEndpoint(strPeerAddress, u16PeerPort);

    //Async connect can have timeout or be cancelled at any point
    m_oSocket.async_connect(oPeerEndPoint,
                            boost::bind(&cInterruptibleBlockingTCPSocket::callback_connectComplete,
                                        this,
                                        boost::asio::placeholders::error)
                            );


    // Setup a deadline time to implement our timeout.
    if(u32Timeout_ms)
    {
        m_oTimer.expires_from_now( boost::posix_time::milliseconds(u32Timeout_ms) );
        m_oTimer.async_wait( boost::bind(&cInterruptibleBlockingTCPSocket::callback_connectTimeOut,
                                         this, boost::asio::placeholders::error) );
    }

    m_oSocket.get_io_service().run();

    if(!m_bError)
        cout << "cInterruptibleBlockingTCPSocket::openAndConnect(): Successfully connected TCP socket to " << strPeerAddress << ":" << u16PeerPort << endl;

    return !m_bError;
}

void cInterruptibleBlockingTCPSocket::close()
{
    //If the socket is open close it
    if(m_oSocket.is_open())
    {
        m_oSocket.cancel();
        m_oSocket.close();
    }
}

bool cInterruptibleBlockingTCPSocket::send(char *cpBuffer, uint32_t u32NBytes, uint32_t u32Timeout_ms)
{
    //Note this function sends to the specific endpoint set in the constructor or with the openAndBind function

    //Necessary after a timeout:
    m_oSocket.get_io_service().reset();

    //Asynchronously write characters
    m_oSocket.async_send( boost::asio::buffer(cpBuffer, u32NBytes),
                          boost::bind(&cInterruptibleBlockingTCPSocket::callback_transferComplete,
                                      this,
                                      boost::asio::placeholders::error,
                                      boost::asio::placeholders::bytes_transferred) );

    // Setup a deadline time to implement our timeout.
    if(u32Timeout_ms)
    {
        m_oTimer.expires_from_now( boost::posix_time::milliseconds(u32Timeout_ms) );
        m_oTimer.async_wait( boost::bind(&cInterruptibleBlockingTCPSocket::callback_transferTimeOut,
                                         this, boost::asio::placeholders::error) );
    }

    // This will block until a character is read
    // or until the it is cancelled.
    m_oSocket.get_io_service().run();

    return !m_bError;
}

bool cInterruptibleBlockingTCPSocket::receive(char *cpBuffer, uint32_t u32NBytes, uint32_t u32Timeout_ms)
{
    //Necessary after a timeout:
    m_oSocket.get_io_service().reset();

    //Asynchronously read characters into string
    m_oSocket.async_receive( boost::asio::buffer(cpBuffer, u32NBytes),
                             boost::bind(&cInterruptibleBlockingTCPSocket::callback_transferComplete,
                                         this,
                                         boost::asio::placeholders::error,
                                         boost::asio::placeholders::bytes_transferred) );

    // Setup a deadline time to implement our timeout.
    if(u32Timeout_ms)
    {
        m_oTimer.expires_from_now(boost::posix_time::milliseconds(u32Timeout_ms));
        m_oTimer.async_wait(boost::bind(&cInterruptibleBlockingTCPSocket::callback_transferTimeOut,
                                        this, boost::asio::placeholders::error));
    }

    // This will block until a byte is read
    // or until the it is cancelled.
    m_oSocket.get_io_service().run();

    fflush(stdout);

    return !m_bError;
}

void cInterruptibleBlockingTCPSocket::callback_connectComplete(const boost::system::error_code& oError)
{
    m_bError = oError;
    m_oTimer.cancel();

    m_oLastError = oError;
}

void cInterruptibleBlockingTCPSocket::callback_connectTimeOut(const boost::system::error_code& oError)
{
    if (oError)
    {
        m_oLastError = oError;
        return;
    }

    std::cout << "!!! Time out reached on socket connect\"" << m_strName << "\" (" << this << ")" << std::endl;

    m_oSocket.cancel();
}

void cInterruptibleBlockingTCPSocket::callback_transferComplete(const boost::system::error_code& oError, uint32_t u32NBytesTransferred)
{
    m_bError = oError || (u32NBytesTransferred == 0);
    m_oTimer.cancel();

    m_u32NBytesLastTransferred = u32NBytesTransferred;
    m_oLastError = oError;
}

void cInterruptibleBlockingTCPSocket::callback_transferTimeOut(const boost::system::error_code& oError)
{
    if (oError)
    {
        m_oLastError = oError;
        return;
    }

    std::cout << "!!! Time out reached on socket transfer\"" << m_strName << "\" (" << this << ")" << std::endl;

    m_oSocket.cancel();
}

void cInterruptibleBlockingTCPSocket::cancelCurrrentOperations()
{
    m_oTimer.cancel();
    m_oSocket.cancel();
}

boost::asio::ip::tcp::endpoint cInterruptibleBlockingTCPSocket::createEndpoint(string strHostAddress, uint16_t u16Port)
{
    stringstream oSS;
    oSS << u16Port;

    return *m_oResolver.resolve(boost::asio::ip::tcp::resolver::query(boost::asio::ip::tcp::v4(), strHostAddress, oSS.str()));
}

std::string cInterruptibleBlockingTCPSocket::getEndpointHostAddress(boost::asio::ip::tcp::endpoint oEndPoint)
{
    return oEndPoint.address().to_string();
}

uint16_t cInterruptibleBlockingTCPSocket::getEndpointPort(boost::asio::ip::tcp::endpoint oEndPoint)
{
    return oEndPoint.port();
}

boost::asio::ip::tcp::endpoint cInterruptibleBlockingTCPSocket::getLocalEndpoint()
{
    return m_oSocket.local_endpoint();
}

std::string cInterruptibleBlockingTCPSocket::getLocalInterface()
{
    return getEndpointHostAddress(m_oSocket.local_endpoint());
}

uint16_t cInterruptibleBlockingTCPSocket::getLocalPort()
{
    return getEndpointPort(m_oSocket.local_endpoint());
}

boost::asio::ip::tcp::endpoint cInterruptibleBlockingTCPSocket::getPeerEndpoint()
{
    return m_oSocket.local_endpoint();
}

std::string cInterruptibleBlockingTCPSocket::getPeerAddress()
{
    return getEndpointHostAddress(m_oSocket.remote_endpoint());
}

uint16_t cInterruptibleBlockingTCPSocket::getPeerPort()
{
    return getEndpointPort(m_oSocket.remote_endpoint());
}

std::string cInterruptibleBlockingTCPSocket::getName()
{
    return m_strName;
}

uint32_t cInterruptibleBlockingTCPSocket::getNBytesLastTransferred()
{
    return m_u32NBytesLastTransferred;
}

boost::system::error_code cInterruptibleBlockingTCPSocket::getLastError()
{
    return m_oLastError;
}

uint32_t cInterruptibleBlockingTCPSocket::getBytesAvailable()
{
    return m_oSocket.available();
}

boost::asio::ip::tcp::socket* cInterruptibleBlockingTCPSocket::getBoostSocketPointer()
{
    return &m_oSocket;
}
