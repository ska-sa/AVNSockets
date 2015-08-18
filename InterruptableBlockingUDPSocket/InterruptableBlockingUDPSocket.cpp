

//System includes
#include <iostream>

//Library includes

//Local includes
#include "InterruptableBlockingUDPSocket.h"

using namespace std;

cInterruptibleBlockUDPSocket::cInterruptibleBlockUDPSocket(const string &strName) :
    m_oSocket(m_oIOService),
    m_oTimer(m_oIOService),
    m_oResolver(m_oIOService),
    m_bError(true),
    m_strName(strName)
{
}

cInterruptibleBlockUDPSocket::cInterruptibleBlockUDPSocket(const string &strRemoteAddress, uint16_t u16RemotePort, const string &strName) :
    m_oSocket(m_oIOService),
    m_oTimer(m_oIOService),
    m_oResolver(m_oIOService),
    m_bError(true),
    m_strName(strName)
{
    openAndConnectSocket(strRemoteAddress, u16RemotePort);
}

void cInterruptibleBlockUDPSocket::openAndBindSocket(string strLocalAddress, uint16_t u16LocalPort)
{
    //Error code to check returns of socket functions
    boost::system::error_code oEC;

    //If the socket is already open close it
    close();

    //Open the socket
    m_oSocket.open(boost::asio::ip::udp::v4(), oEC);

    //Set some socket options
    m_oSocket.set_option( boost::asio::socket_base::receive_buffer_size(64 * 1024 * 1024) ); //Set buffer to 64 MB
    m_oSocket.set_option( boost::asio::socket_base::reuse_address(true) );

    m_oBoundLocalEndPoint = createEndPoint(strLocalAddress, u16LocalPort);

    if(oEC)
    {
        cout << "Error opening socket: " << oEC.message() << endl;
    }
    else
    {
        cout << "Successfully opening UDP socket.";
    }

    m_oSocket.bind(m_oBoundLocalEndPoint, oEC);
    if (oEC)
    {
        m_oLastError = oEC;
        cout << "Error binding socket: " << oEC.message() << endl;
    }
    else
    {
        cout << "Successfully bound UDP socket to " << getBoundLocalAddress() << ":" << getBoundLocalPort() << endl;
    }
}

void cInterruptibleBlockUDPSocket::openAndConnectSocket(string strRemoteAddress, uint16_t u16RemotePort)
{
    boost::system::error_code oEC;

    //If the socket is already open close it
    close();

    //Open the socket
    m_oSocket.open(boost::asio::ip::udp::v4(), oEC);

    //Set some socket options
    m_oSocket.set_option( boost::asio::socket_base::receive_buffer_size(64 * 1024 * 1024) ); //Set buffer to 64 MB
    m_oSocket.set_option( boost::asio::socket_base::reuse_address(true) );

    m_oConnectedRemoteEndPoint = createEndPoint(strRemoteAddress, u16RemotePort);

    if(oEC)
    {
        cout << "Error opening socket: " << oEC.message() << endl;
    }
    else
    {
        cout << "Successfully opened UDP socket.";
    }

    m_oSocket.connect(m_oConnectedRemoteEndPoint, oEC);
    if (oEC)
    {
        m_oLastError = oEC;
        cout << "Error binding socket: " << oEC.message() << endl;
    }
    else
    {
        cout << "Successfully connected UDP socket to " << getConnectedRemoteAddress() << ":" << getConnectedRemotePort() << endl;
    }
}

void cInterruptibleBlockUDPSocket::close()
{
    //If the socket is open close it
    if(m_oSocket.is_open())
    {
        cout << "Socket is already open. Closing first." << endl;
        m_oSocket.cancel();
        m_oSocket.close();
    }
}

bool cInterruptibleBlockUDPSocket::send(char *cpBuffer, uint32_t u32NBytes, uint32_t u32Timeout_ms)
{
    //Note this function sends to the specific endpoint set in the constructor or with the openAndBind function

    //Necessary after a timeout:
    m_oSocket.get_io_service().reset();

    //Asynchronously write characters
    m_oSocket.async_send( boost::asio::buffer(cpBuffer, u32NBytes),
                             boost::bind(&cInterruptibleBlockUDPSocket::callback_complete,
                                         this,
                                         boost::asio::placeholders::error,
                                         boost::asio::placeholders::bytes_transferred) );

    // Setup a deadline time to implement our timeout.
    if(u32Timeout_ms)
    {
        m_oTimer.expires_from_now( boost::posix_time::milliseconds(u32Timeout_ms) );
        m_oTimer.async_wait( boost::bind(&cInterruptibleBlockUDPSocket::callback_timeOut,
                                         this, boost::asio::placeholders::error) );
    }

    // This will block until a character is read
    // or until the it is cancelled.
    m_oSocket.get_io_service().run();

    return !m_bError;
}

bool cInterruptibleBlockUDPSocket::sendTo(char *cpBuffer, uint32_t u32NBytes, const std::string &strRemoteHost, uint16_t u16RemotePort, uint32_t u32Timeout_ms)
{
    return cInterruptibleBlockUDPSocket::sendTo(cpBuffer, u32NBytes, createEndPoint(strRemoteHost, u16RemotePort), u32Timeout_ms);
}

bool cInterruptibleBlockUDPSocket::sendTo(char *cpBuffer, uint32_t u32NBytes, const boost::asio::ip::udp::endpoint &oRemoteEndPoint, uint32_t u32Timeout_ms)
{
    //Note this function sends to the specific endpoint set in the constructor or with the openAndBind function

    //Necessary after a timeout:
    m_oSocket.get_io_service().reset();

    //Asynchronously write characters
    m_oSocket.async_send_to( boost::asio::buffer(cpBuffer, u32NBytes),
                             oRemoteEndPoint,
                             boost::bind(&cInterruptibleBlockUDPSocket::callback_complete,
                                         this,
                                         boost::asio::placeholders::error,
                                         boost::asio::placeholders::bytes_transferred) );

    // Setup a deadline time to implement our timeout.
    if(u32Timeout_ms)
    {
        m_oTimer.expires_from_now( boost::posix_time::milliseconds(u32Timeout_ms) );
        m_oTimer.async_wait( boost::bind(&cInterruptibleBlockUDPSocket::callback_timeOut,
                                         this, boost::asio::placeholders::error) );
    }

    // This will block until a character is read
    // or until the it is cancelled.
    m_oSocket.get_io_service().run();

    return !m_bError;
}

bool cInterruptibleBlockUDPSocket::receive(char *cpBuffer, uint32_t u32NBytes, uint32_t u32Timeout_ms)
{
    //Necessary after a timeout:
    m_oSocket.get_io_service().reset();

    //Asynchronously read characters into string
    m_oSocket.async_receive( boost::asio::buffer(cpBuffer, u32NBytes),
                                  boost::bind(&cInterruptibleBlockUDPSocket::callback_complete,
                                              this,
                                              boost::asio::placeholders::error,
                                              boost::asio::placeholders::bytes_transferred) );

    // Setup a deadline time to implement our timeout.
    if(u32Timeout_ms)
    {
        m_oTimer.expires_from_now(boost::posix_time::milliseconds(u32Timeout_ms));
        m_oTimer.async_wait(boost::bind(&cInterruptibleBlockUDPSocket::callback_timeOut,
                                        this, boost::asio::placeholders::error));
    }

    // This will block until a byte is read
    // or until the it is cancelled.
    m_oSocket.get_io_service().run();

    return !m_bError;
}

bool cInterruptibleBlockUDPSocket::receiveFrom(char *cpBuffer, uint32_t u32NBytes, std::string &strRemoteHost, uint16_t &u16RemotePort, uint32_t u32Timeout_ms)
{
    boost::asio::ip::udp::endpoint oRemoteEndPoint;
    bool bResult = receiveFrom(cpBuffer, u32NBytes, oRemoteEndPoint, u32Timeout_ms);

    strRemoteHost = getEndPointHostAddress(oRemoteEndPoint);
    u16RemotePort = getEndPointPort(oRemoteEndPoint);

    return bResult;
}

bool cInterruptibleBlockUDPSocket::receiveFrom(char *cpBuffer, uint32_t u32NBytes, boost::asio::ip::udp::endpoint &oRemoteEndPoint, uint32_t u32Timeout_ms)
{
    //Necessary after a timeout:
    m_oSocket.get_io_service().reset();

    //Asynchronously read characters into string
    m_oSocket.async_receive_from( boost::asio::buffer(cpBuffer, u32NBytes),
                                  oRemoteEndPoint,
                                  boost::bind(&cInterruptibleBlockUDPSocket::callback_complete,
                                              this,
                                              boost::asio::placeholders::error,
                                              boost::asio::placeholders::bytes_transferred) );

    // Setup a deadline time to implement our timeout.
    if(u32Timeout_ms)
    {
        m_oTimer.expires_from_now(boost::posix_time::milliseconds(u32Timeout_ms));
        m_oTimer.async_wait(boost::bind(&cInterruptibleBlockUDPSocket::callback_timeOut,
                                        this, boost::asio::placeholders::error));
    }

    // This will block until a byte is read
    // or until the it is cancelled.
    m_oSocket.get_io_service().run();

    return !m_bError;
}


void cInterruptibleBlockUDPSocket::callback_complete(const boost::system::error_code& oError, uint32_t u32NBytesTransferred)
{
    m_bError = oError || (u32NBytesTransferred == 0);
    m_oTimer.cancel();

    m_u32NBytesLastTransferred = u32NBytesTransferred;
    m_oLastError = oError;
}

void cInterruptibleBlockUDPSocket::callback_timeOut(const boost::system::error_code& oError)
{
    if (oError)
    {
        m_oLastError = oError;
        return;
    }

    std::cout << "!!! Time out reached on socket \"" << m_strName << "\" (" << this << ")" << std::endl;

    m_oSocket.cancel();
}

void cInterruptibleBlockUDPSocket::cancelCurrrentOperations()
{
    m_oTimer.cancel();
    m_oSocket.cancel();
}

boost::asio::ip::udp::endpoint cInterruptibleBlockUDPSocket::createEndPoint(string strHostAddress, uint16_t u16Port)
{
    stringstream oSS;
    oSS << u16Port;

    return *m_oResolver.resolve(boost::asio::ip::udp::resolver::query(boost::asio::ip::udp::v4(), strHostAddress, oSS.str()));
}

std::string cInterruptibleBlockUDPSocket::getEndPointHostAddress(boost::asio::ip::udp::endpoint oEndPoint)
{
    return oEndPoint.address().to_string();
}

uint16_t cInterruptibleBlockUDPSocket::getEndPointPort(boost::asio::ip::udp::endpoint oEndPoint)
{
    return oEndPoint.port();
}

boost::asio::ip::udp::endpoint cInterruptibleBlockUDPSocket::getBoundLocalEndpoint()
{
    return m_oBoundLocalEndPoint;
}

std::string cInterruptibleBlockUDPSocket::getBoundLocalAddress()
{
    return getEndPointHostAddress(m_oBoundLocalEndPoint);
}

uint16_t cInterruptibleBlockUDPSocket::getBoundLocalPort()
{
    return getEndPointPort(m_oBoundLocalEndPoint);
}

boost::asio::ip::udp::endpoint cInterruptibleBlockUDPSocket::getConnectedRemoteEndPoint()
{
    return m_oConnectedRemoteEndPoint;
}

std::string cInterruptibleBlockUDPSocket::getConnectedRemoteAddress()
{
    return getEndPointHostAddress(m_oConnectedRemoteEndPoint);
}

uint16_t cInterruptibleBlockUDPSocket::getConnectedRemotePort()
{
    return getEndPointPort(m_oConnectedRemoteEndPoint);
}

std::string cInterruptibleBlockUDPSocket::getName()
{
    return m_strName;
}

uint32_t cInterruptibleBlockUDPSocket::getNBytesLastTransferred()
{
    return m_u32NBytesLastTransferred;
}

boost::system::error_code cInterruptibleBlockUDPSocket::getLastError()
{
    return m_oLastError;
}

uint32_t cInterruptibleBlockUDPSocket::getBytesAvailable()
{
    return m_oSocket.available();
}
