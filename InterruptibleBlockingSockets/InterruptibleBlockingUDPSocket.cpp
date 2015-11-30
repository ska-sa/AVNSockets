

//System includes
#include <iostream>

//Library includes
#ifndef Q_MOC_RUN //Qt's MOC and Boost have some issues don't let MOC process boost headers
#include <boost/bind.hpp>
#include <boost/asio/placeholders.hpp>
#endif

//Local includes
#include "InterruptibleBlockingUDPSocket.h"

using namespace std;

cInterruptibleBlockingUDPSocket::cInterruptibleBlockingUDPSocket(const string &strName) :
    m_oSocket(m_oIOService),
    m_oTimer(m_oIOService),
    m_oResolver(m_oIOService),
    m_bError(true),
    m_strName(strName)
{
}

cInterruptibleBlockingUDPSocket::cInterruptibleBlockingUDPSocket(const string &strLocalInterface, uint16_t u16LocalPort, const string &strPeerAddress, uint16_t u16PeerPort, const string &strName) :
    m_oSocket(m_oIOService),
    m_oTimer(m_oIOService),
    m_oResolver(m_oIOService),
    m_bError(true),
    m_strName(strName)
{
    if(strPeerAddress.length())
        openBindAndConnect(strLocalInterface, u16LocalPort, strPeerAddress, u16PeerPort);
    else
        openAndBind(strLocalInterface, u16LocalPort);

}

cInterruptibleBlockingUDPSocket::~cInterruptibleBlockingUDPSocket()
{
    close();
}

bool cInterruptibleBlockingUDPSocket::openAndBind(const string &strLocalAddress, uint16_t u16LocalPort)
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

    m_oLocalEndpoint = createEndpoint(strLocalAddress, u16LocalPort);

    if(oEC)
    {
        cout << "cInterruptibleBlockingUDPSocket::openAndBind(): Error opening socket: " << oEC.message() << endl;
        return false;
    }
    else
    {
        cout << "cInterruptibleBlockingUDPSocket::openAndBind(): Successfully opened UDP socket." << endl;
    }

    m_oSocket.bind(m_oLocalEndpoint, oEC);
    if (oEC)
    {
        m_oLastError = oEC;
        cout << "Error binding socket: " << oEC.message() << endl;
        return false;
    }
    else
    {
        cout << "cInterruptibleBlockingUDPSocket::openAndBind(): Successfully bound UDP socket to " << getLocalInterface() << ":" << getLocalPort() << endl;
    }
    return true;
}

bool cInterruptibleBlockingUDPSocket::openBindAndConnect(const string &strLocalInterface, uint16_t u16LocalPort, const string &strPeerAddress, uint16_t u16PeerPort)
{
    boost::system::error_code oEC;

    openAndBind(strLocalInterface, u16LocalPort);

    m_oPeerEndpoint = createEndpoint(strPeerAddress, u16PeerPort);

    if(oEC)
    {
        cout << "Error opening socket: " << oEC.message() << endl;
        return false;
    }
    else
    {
        cout << "Successfully opened UDP socket." << endl;
    }

    m_oSocket.connect(m_oPeerEndpoint, oEC);
    if (oEC)
    {
        m_oLastError = oEC;
        cout << "Error binding socket: " << oEC.message() << endl;
        return false;
    }
    else
    {
        cout << "Successfully created virtual UDP connection to " << getPeerAddress() << ":" << getPeerPort() << endl;
    }
    return true;
}

void cInterruptibleBlockingUDPSocket::close()
{
    cout << "cInterruptibleBlockingUDPSocket::close(): Cancelling all current socket operations." << endl;
    cancelCurrrentOperations();

    //If the socket is open close it
    if(m_oSocket.is_open())
    {
        try
        {
            m_oSocket.close();
        }
        catch(boost::system::system_error &e)
        {
            //Catch special conditions where socket is trying to be opened etc.
            //Prevents crash.
        }
    }
}

bool cInterruptibleBlockingUDPSocket::send(const char *cpBuffer, uint32_t u32NBytes, uint32_t u32Timeout_ms)
{
    //Note this function sends to the specific endpoint set in the constructor or with the openAndBind function

    //Necessary after a timeout:
    m_oSocket.get_io_service().reset();

    //Asynchronously write characters
    m_oSocket.async_send( boost::asio::buffer(cpBuffer, u32NBytes),
                          boost::bind(&cInterruptibleBlockingUDPSocket::callback_complete,
                                      this,
                                      boost::asio::placeholders::error,
                                      boost::asio::placeholders::bytes_transferred) );

    // Setup a deadline time to implement our timeout.
    if(u32Timeout_ms)
    {
        m_oTimer.expires_from_now( boost::posix_time::milliseconds(u32Timeout_ms) );
        m_oTimer.async_wait( boost::bind(&cInterruptibleBlockingUDPSocket::callback_timeOut,
                                         this, boost::asio::placeholders::error) );
    }

    // This will block until a character is read
    // or until the it is cancelled.
    m_oSocket.get_io_service().run();

    return !m_bError;
}

bool cInterruptibleBlockingUDPSocket::sendTo(const char *cpBuffer, uint32_t u32NBytes, const std::string &strPeerAddress, uint16_t u16PeerPort, uint32_t u32Timeout_ms)
{
    return cInterruptibleBlockingUDPSocket::sendTo(cpBuffer, u32NBytes, createEndpoint(strPeerAddress, u16PeerPort), u32Timeout_ms);
}

bool cInterruptibleBlockingUDPSocket::sendTo(const char *cpBuffer, uint32_t u32NBytes, const boost::asio::ip::udp::endpoint &oPeerEndpoint, uint32_t u32Timeout_ms)
{
    //Note this function sends to the specific endpoint set in the constructor or with the openAndBind function

    //Necessary after a timeout:
    m_oSocket.get_io_service().reset();

    //Asynchronously write characters
    m_oSocket.async_send_to( boost::asio::buffer(cpBuffer, u32NBytes),
                             oPeerEndpoint,
                             boost::bind(&cInterruptibleBlockingUDPSocket::callback_complete,
                                         this,
                                         boost::asio::placeholders::error,
                                         boost::asio::placeholders::bytes_transferred) );

    // Setup a deadline time to implement our timeout.
    if(u32Timeout_ms)
    {
        m_oTimer.expires_from_now( boost::posix_time::milliseconds(u32Timeout_ms) );
        m_oTimer.async_wait( boost::bind(&cInterruptibleBlockingUDPSocket::callback_timeOut,
                                         this, boost::asio::placeholders::error) );
    }

    // This will block until a character is read
    // or until the it is cancelled.
    m_oSocket.get_io_service().run();

    return !m_bError;
}

bool cInterruptibleBlockingUDPSocket::receive(char *cpBuffer, uint32_t u32NBytes, uint32_t u32Timeout_ms)
{
    //Necessary after a timeout:
    m_oSocket.get_io_service().reset();

    //Asynchronously read characters into string
    m_oSocket.async_receive( boost::asio::buffer(cpBuffer, u32NBytes),
                             boost::bind(&cInterruptibleBlockingUDPSocket::callback_complete,
                                         this,
                                         boost::asio::placeholders::error,
                                         boost::asio::placeholders::bytes_transferred) );

    // Setup a deadline time to implement our timeout.
    if(u32Timeout_ms)
    {
        m_oTimer.expires_from_now(boost::posix_time::milliseconds(u32Timeout_ms));
        m_oTimer.async_wait(boost::bind(&cInterruptibleBlockingUDPSocket::callback_timeOut,
                                        this, boost::asio::placeholders::error));
    }

    // This will block until a byte is read
    // or until the it is cancelled.
    m_oSocket.get_io_service().run();

    return !m_bError;
}

bool cInterruptibleBlockingUDPSocket::receiveFrom(char *cpBuffer, uint32_t u32NBytes, std::string &strPeerAddress, uint16_t &u16PeerPort, uint32_t u32Timeout_ms)
{
    boost::asio::ip::udp::endpoint oPeerEndpoint;
    bool bResult = receiveFrom(cpBuffer, u32NBytes, oPeerEndpoint, u32Timeout_ms);

    strPeerAddress = getEndpointHostAddress(oPeerEndpoint);
    u16PeerPort = getEndpointPort(oPeerEndpoint);

    return bResult;
}

bool cInterruptibleBlockingUDPSocket::receiveFrom(char *cpBuffer, uint32_t u32NBytes, boost::asio::ip::udp::endpoint &oPeerEndpoint, uint32_t u32Timeout_ms)
{
    //Necessary after a timeout:
    m_oSocket.get_io_service().reset();

    //Asynchronously read characters into string
    m_oSocket.async_receive_from( boost::asio::buffer(cpBuffer, u32NBytes),
                                  oPeerEndpoint,
                                  boost::bind(&cInterruptibleBlockingUDPSocket::callback_complete,
                                              this,
                                              boost::asio::placeholders::error,
                                              boost::asio::placeholders::bytes_transferred) );

    // Setup a deadline time to implement our timeout.
    if(u32Timeout_ms)
    {
        m_oTimer.expires_from_now(boost::posix_time::milliseconds(u32Timeout_ms));
        m_oTimer.async_wait(boost::bind(&cInterruptibleBlockingUDPSocket::callback_timeOut,
                                        this, boost::asio::placeholders::error));
    }

    // This will block until a byte is read
    // or until the it is cancelled.
    m_oSocket.get_io_service().run();

    return !m_bError;
}


void cInterruptibleBlockingUDPSocket::callback_complete(const boost::system::error_code& oError, uint32_t u32NBytesTransferred)
{
    m_bError = oError || (u32NBytesTransferred == 0);
    m_oTimer.cancel();

    m_u32NBytesLastTransferred = u32NBytesTransferred;
    m_oLastError = oError;
}

void cInterruptibleBlockingUDPSocket::callback_timeOut(const boost::system::error_code& oError)
{
    if (oError)
    {
        m_oLastError = oError;
        return;
    }

    std::cout << "!!! Time out reached on socket \"" << m_strName << "\" (" << this << ")" << std::endl;

    m_oSocket.cancel();
}

void cInterruptibleBlockingUDPSocket::cancelCurrrentOperations()
{   
    try
    {
        m_oSocket.get_io_service().stop();
        m_oSocket.cancel();
        m_oTimer.cancel();
    }
    catch(boost::system::system_error &e)
    {
        //Catch special conditions where socket is trying to be opened etc.
        //Prevents crash.
    }
}

boost::asio::ip::udp::endpoint cInterruptibleBlockingUDPSocket::createEndpoint(string strHostAddress, uint16_t u16Port)
{
    stringstream oSS;
    oSS << u16Port;

    return *m_oResolver.resolve(boost::asio::ip::udp::resolver::query(boost::asio::ip::udp::v4(), strHostAddress, oSS.str()));
}

std::string cInterruptibleBlockingUDPSocket::getEndpointHostAddress(boost::asio::ip::udp::endpoint oEndpoint) const
{
    return oEndpoint.address().to_string();
}

uint16_t cInterruptibleBlockingUDPSocket::getEndpointPort(boost::asio::ip::udp::endpoint oEndpoint) const
{
    return oEndpoint.port();
}

boost::asio::ip::udp::endpoint cInterruptibleBlockingUDPSocket::getLocalEndpoint() const
{
    return m_oLocalEndpoint;
}

std::string cInterruptibleBlockingUDPSocket::getLocalInterface() const
{
    return getEndpointHostAddress(m_oLocalEndpoint);
}

uint16_t cInterruptibleBlockingUDPSocket::getLocalPort() const
{
    return getEndpointPort(m_oLocalEndpoint);
}

boost::asio::ip::udp::endpoint cInterruptibleBlockingUDPSocket::getPeerEndpoint() const
{
    return m_oPeerEndpoint;
}

std::string cInterruptibleBlockingUDPSocket::getPeerAddress() const
{
    return getEndpointHostAddress(m_oPeerEndpoint);
}

uint16_t cInterruptibleBlockingUDPSocket::getPeerPort() const
{
    return getEndpointPort(m_oPeerEndpoint);
}

std::string cInterruptibleBlockingUDPSocket::getName() const
{
    return m_strName;
}

uint32_t cInterruptibleBlockingUDPSocket::getNBytesLastTransferred() const
{
    return m_u32NBytesLastTransferred;
}

boost::system::error_code cInterruptibleBlockingUDPSocket::getLastError() const
{
    return m_oLastError;
}

uint32_t cInterruptibleBlockingUDPSocket::getBytesAvailable() const
{
    return m_oSocket.available();
}

boost::asio::ip::udp::socket* cInterruptibleBlockingUDPSocket::getBoostSocketPointer()
{
    return &m_oSocket;
}

