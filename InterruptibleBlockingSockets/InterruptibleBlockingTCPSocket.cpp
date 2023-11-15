
//System includes
#include <iostream>

//Library includes
#ifndef Q_MOC_RUN //Qt's MOC and Boost have some issues don't let MOC process boost headers
#include <boost/bind.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#endif

//Local includes
#include "InterruptibleBlockingTCPSocket.h"

using namespace std;

cInterruptibleBlockingTCPSocket::cInterruptibleBlockingTCPSocket(const string &strName) :
    m_oSocket(m_oIOService),
    m_oOpenAndConnectTimer(m_oIOService),
    m_oReadTimer(m_oIOService),
    m_oWriteTimer(m_oIOService),
    m_oResolver(m_oIOService),
    m_bOpenAndConnectError(true),
    m_bReadError(true),
    m_bWriteError(true),
    m_strName(strName)
{
}

cInterruptibleBlockingTCPSocket::cInterruptibleBlockingTCPSocket(const string &strRemoteAddress, uint16_t u16RemotePort, const string &strName) :
    m_oSocket(m_oIOService),
    m_oOpenAndConnectTimer(m_oIOService),
    m_oReadTimer(m_oIOService),
    m_oWriteTimer(m_oIOService),
    m_oResolver(m_oIOService),
    m_bOpenAndConnectError(true),
    m_bReadError(true),
    m_bWriteError(true),
    m_strName(strName)
{
    openAndConnect(strRemoteAddress, u16RemotePort);
}

cInterruptibleBlockingTCPSocket::~cInterruptibleBlockingTCPSocket()
{
    close();
}

bool cInterruptibleBlockingTCPSocket::openAndConnect(string strPeerAddress, uint16_t u16PeerPort, uint32_t u32Timeout_ms)
{
    if(m_oSocket.get_io_service().stopped())
    {
        //Necessary after a timeout or previously finished run:
        m_oSocket.get_io_service().reset();
    }

    //If the socket is already open close it
    close();

    //Open the socket
    m_oSocket.open(boost::asio::ip::tcp::v4(), m_oLastopenAndConnectError);

    if(m_oLastopenAndConnectError)
    {
        cout << "cInterruptibleBlockingTCPSocket::openAndConnect(): Error opening socket: " << m_oLastopenAndConnectError.message() << endl;
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

    boost::asio::ip::tcp::endpoint oPeerEndPoint;
    try
    {
        oPeerEndPoint = createEndpoint(strPeerAddress, u16PeerPort);
    }
    catch(boost::system::system_error &e)
    {
        //Typically thrown when hostname cannot be resolved.
        return false;
    }

    //Async connect can have timeout or be cancelled at any point
    m_oSocket.async_connect(oPeerEndPoint,
                            boost::bind(&cInterruptibleBlockingTCPSocket::callback_connectComplete,
                                        this,
                                        boost::asio::placeholders::error)
                            );


    // Setup a deadline time to implement our timeout.
    if(u32Timeout_ms)
    {
        m_oOpenAndConnectTimer.expires_from_now( boost::posix_time::milliseconds(u32Timeout_ms) );
        m_oOpenAndConnectTimer.async_wait( boost::bind(&cInterruptibleBlockingTCPSocket::callback_connectTimeOut,
                                                       this, boost::asio::placeholders::error) );
    }

    m_oSocket.get_io_service().run();

    if(!m_bOpenAndConnectError)
        cout << "cInterruptibleBlockingTCPSocket::openAndConnect(): Successfully connected TCP socket to " << strPeerAddress << ":" << u16PeerPort << endl;

    return !m_bOpenAndConnectError;
}

void cInterruptibleBlockingTCPSocket::close()
{
    //If the socket is open close it
    if(m_oSocket.is_open())
    {
        m_oSocket.cancel();

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

bool cInterruptibleBlockingTCPSocket::send(const char *cpBuffer, uint32_t u32NBytes, uint32_t u32Timeout_ms)
{
    //Note this function sends to the specific endpoint set in the constructor or with the openAndBind function

    boost::unique_lock<boost::mutex> oLock(m_oMutex);

    if(m_oSocket.get_io_service().stopped())
    {
        //Necessary after a timeout or previously finished run:
        m_oSocket.get_io_service().reset();
    }

    //Asynchronously write characters
    m_oSocket.async_send( boost::asio::buffer(cpBuffer, u32NBytes),
                          boost::bind(&cInterruptibleBlockingTCPSocket::callback_writeComplete,
                                      this,
                                      boost::asio::placeholders::error,
                                      boost::asio::placeholders::bytes_transferred) );

    // Setup a deadline time to implement our timeout.
    if(u32Timeout_ms)
    {
        m_oWriteTimer.expires_from_now( boost::posix_time::milliseconds(u32Timeout_ms) );
        m_oWriteTimer.async_wait( boost::bind(&cInterruptibleBlockingTCPSocket::callback_writeTimeOut,
                                              this, boost::asio::placeholders::error) );
    }

    // This will block until at least a byte is written
    // or until it is cancelled.
    m_oSocket.get_io_service().run();

    return !m_bWriteError;
}

bool cInterruptibleBlockingTCPSocket::receive(char *cpBuffer, uint32_t u32NBytes, uint32_t u32Timeout_ms)
{
    boost::unique_lock<boost::mutex> oLock(m_oMutex);

    if(m_oSocket.get_io_service().stopped())
    {
        //Necessary after a timeout or previously finished run:
        m_oSocket.get_io_service().reset();
    }

    //Asynchronously read characters into string
    m_oSocket.async_receive( boost::asio::buffer(cpBuffer, u32NBytes),
                             boost::bind(&cInterruptibleBlockingTCPSocket::callback_readComplete,
                                         this,
                                         boost::asio::placeholders::error,
                                         boost::asio::placeholders::bytes_transferred) );

    // Setup a deadline time to implement our timeout.
    if(u32Timeout_ms)
    {
        m_oReadTimer.expires_from_now(boost::posix_time::milliseconds(u32Timeout_ms));
        m_oReadTimer.async_wait(boost::bind(&cInterruptibleBlockingTCPSocket::callback_readTimeOut,
                                            this, boost::asio::placeholders::error));
    }

    // This will block until at least a byte is read
    // or until it is cancelled.
    m_oSocket.get_io_service().run();

    return !m_bReadError;
}

bool cInterruptibleBlockingTCPSocket::write(const char *cpBuffer, uint32_t u32NBytes, uint32_t u32Timeout_ms)
{
    boost::unique_lock<boost::mutex> oLock(m_oMutex);

    //The write function guarantees deliver of all u32NBytes bytes in send buffer unless and error is encountered

    if(m_oSocket.get_io_service().stopped())
    {
        //Necessary after a timeout or previously finished run:
        m_oSocket.get_io_service().reset();
    }

    //Asynchronously write all data
    boost::asio::async_write(m_oSocket, boost::asio::buffer(cpBuffer, u32NBytes),
                             boost::bind(&cInterruptibleBlockingTCPSocket::callback_writeComplete,
                                         this,
                                         boost::asio::placeholders::error,
                                         boost::asio::placeholders::bytes_transferred) );

    // Setup a deadline time to implement our timeout.
    if(u32Timeout_ms)
    {
        m_oWriteTimer.expires_from_now(boost::posix_time::milliseconds(u32Timeout_ms));
        m_oWriteTimer.async_wait(boost::bind(&cInterruptibleBlockingTCPSocket::callback_writeTimeOut,
                                             this, boost::asio::placeholders::error));
    }

    // This will block until all bytes are written
    // or until it is cancelled.
    m_oSocket.get_io_service().run();

    return !m_bWriteError;
}

bool cInterruptibleBlockingTCPSocket::write(const std::string &strData, uint32_t u32Timeout_ms)
{
    return write(strData.c_str(), strData.length(), u32Timeout_ms);
}

bool cInterruptibleBlockingTCPSocket::read(char *cpBuffer, uint32_t u32NBytes, uint32_t u32Timeout_ms)
{
    boost::unique_lock<boost::mutex> oLock(m_oMutex);

    //The read function guarantees reading of all u32NBytes bytes to buffer unless an error is encountered

    if(m_oSocket.get_io_service().stopped())
    {
        //Necessary after a timeout or previously finished run:
        m_oSocket.get_io_service().reset();
    }

    //Asynchronously read until the delimiting character is found
    boost::asio::async_read(m_oSocket, boost::asio::buffer(cpBuffer, u32NBytes),
                            boost::bind(&cInterruptibleBlockingTCPSocket::callback_readComplete,
                                        this,
                                        boost::asio::placeholders::error,
                                        boost::asio::placeholders::bytes_transferred) );

    // Setup a deadline time to implement our timeout.
    if(u32Timeout_ms)
    {
        m_oReadTimer.expires_from_now(boost::posix_time::milliseconds(u32Timeout_ms));
        m_oReadTimer.async_wait(boost::bind(&cInterruptibleBlockingTCPSocket::callback_readTimeOut,
                                            this, boost::asio::placeholders::error));
    }

    // This will block until all bytes are read
    // or until it is cancelled.
    m_oSocket.get_io_service().run();

    return !m_bReadError;
}

bool cInterruptibleBlockingTCPSocket::readUntil(string &strBuffer, const string &strDelimiter, uint32_t u32Timeout_ms)
{
    boost::unique_lock<boost::mutex> oLock(m_oMutex);

    //Check if we have already read up the delimeter if so return this string
    if(m_strReadUntilBuff.find_first_of(strDelimiter) != string::npos)
    {
        uint32_t u32DelimPos = m_strReadUntilBuff.find_first_of(strDelimiter);
        strBuffer.append(m_strReadUntilBuff.substr(0, u32DelimPos + 1));
        m_strReadUntilBuff.erase(0, u32DelimPos + 1);

        return true;
    }

    if(m_oSocket.get_io_service().stopped())
    {
        //Necessary after a timeout or previously finished run:
        m_oSocket.get_io_service().reset();
    }

    boost::asio::streambuf oStreamBuf;

    //Asynchronously read until the delimiting character is found
    boost::asio::async_read_until(m_oSocket, oStreamBuf, strDelimiter,
                                  boost::bind(&cInterruptibleBlockingTCPSocket::callback_readComplete,
                                              this,
                                              boost::asio::placeholders::error,
                                              boost::asio::placeholders::bytes_transferred) );

    // Setup a deadline time to implement our timeout.
    if(u32Timeout_ms)
    {
        m_oReadTimer.expires_from_now(boost::posix_time::milliseconds(u32Timeout_ms));
        m_oReadTimer.async_wait(boost::bind(&cInterruptibleBlockingTCPSocket::callback_readTimeOut,
                                            this, boost::asio::placeholders::error));
    }

    // This will block until the delimiter is found and read
    // or until the it is cancelled.
    for(;;)
    {
        try
        {
            m_oSocket.get_io_service().run();
            break;
        }
        catch(...)
        {
            cout << "cInterruptibleBlockingTCPSocket::readUntil(): Caught exception on io_service::run()" << endl;
        }
    }

    //Copy the data to the member string. This may contain more than 1 of the delimiter
    try
    {
        m_strReadUntilBuff.append( std::string( (std::istreambuf_iterator<char>(&oStreamBuf)), std::istreambuf_iterator<char>() ) );
    }
    catch(...)
    {
        cout << "cInterruptibleBlockingTCPSocket::readUntil(): Got string convertion error." << endl;
    }

    //Move characters up to the first instance of the delimiter to the argument string
    uint32_t u32DelimPos = m_strReadUntilBuff.find_first_of(strDelimiter);
    strBuffer.append(m_strReadUntilBuff.substr(0, u32DelimPos + 1));
    m_strReadUntilBuff.erase(0, u32DelimPos + 1);

    //Debug: Deallocation of the streambuffer seems segfault sometimes. Try empty first:
    oStreamBuf.consume(oStreamBuf.size());

    return !m_bReadError;
}

void cInterruptibleBlockingTCPSocket::callback_connectComplete(const boost::system::error_code& oError)
{
    m_bOpenAndConnectError= true;
    if (boost::system::errc::success == oError)
    {
      m_bOpenAndConnectError= false;
    }
    //RE: m_bOpenAndConnectError = oError;
    m_oOpenAndConnectTimer.cancel();

    m_oLastopenAndConnectError = oError;
}

void cInterruptibleBlockingTCPSocket::callback_connectTimeOut(const boost::system::error_code& oError)
{
    if (oError)
    {
        m_oLastopenAndConnectError = oError;
        return;
    }

    std::cout << "!!! Time out reached on socket connect\"" << m_strName << "\" (" << this << ")" << std::endl;

    m_oSocket.cancel();
}

void cInterruptibleBlockingTCPSocket::callback_readComplete(const boost::system::error_code& oError, uint32_t u32NBytesTransferred)
{
    m_bReadError = oError || (u32NBytesTransferred == 0);
    m_oReadTimer.cancel();

    m_u32NBytesLastRead = u32NBytesTransferred;
    m_oLastReadError = oError;
}

void cInterruptibleBlockingTCPSocket::callback_writeComplete(const boost::system::error_code& oError, uint32_t u32NBytesTransferred)
{
    m_bWriteError = oError || (u32NBytesTransferred == 0);
    m_oWriteTimer.cancel();

    m_u32NBytesLastWritten = u32NBytesTransferred;
    m_oLastWriteError = oError;
}

void cInterruptibleBlockingTCPSocket::callback_readTimeOut(const boost::system::error_code& oError)
{
    if (oError)
    {
        m_oLastReadTimeoutError = oError;
        return;
    }

    m_oSocket.cancel();
}

void cInterruptibleBlockingTCPSocket::callback_writeTimeOut(const boost::system::error_code& oError)
{
    if (oError)
    {
        m_oLastWriteError = oError;
        return;
    }

    m_oSocket.cancel();
}

void cInterruptibleBlockingTCPSocket::cancelCurrrentOperations()
{
    try
    {
        m_oSocket.get_io_service().stop();
        m_oSocket.cancel();
    }
    catch(boost::system::system_error &e)
    {
        //Catch special conditions where socket is trying to be opened etc.
        //Prevents crash.
    }

    try
    {
        m_oReadTimer.cancel();
    }
    catch(boost::system::system_error &e)
    {
        //Catch special conditions where socket is trying to be opened etc.
        //Prevents crash.
    }

    try
    {
        m_oWriteTimer.cancel();
    }
    catch(boost::system::system_error &e)
    {
        //Catch special conditions where socket is trying to be opened etc.
        //Prevents crash.
    }
}

boost::asio::ip::tcp::endpoint cInterruptibleBlockingTCPSocket::createEndpoint(string strHostAddress, uint16_t u16Port)
{
    stringstream oSS;
    oSS << u16Port;

    return *m_oResolver.resolve(boost::asio::ip::tcp::resolver::query(boost::asio::ip::tcp::v4(), strHostAddress, oSS.str()));
}

std::string cInterruptibleBlockingTCPSocket::getEndpointHostAddress(boost::asio::ip::tcp::endpoint oEndPoint) const
{
    return oEndPoint.address().to_string();
}

uint16_t cInterruptibleBlockingTCPSocket::getEndpointPort(boost::asio::ip::tcp::endpoint oEndPoint) const
{
    return oEndPoint.port();
}

boost::asio::ip::tcp::endpoint cInterruptibleBlockingTCPSocket::getLocalEndpoint() const
{
    return m_oSocket.local_endpoint();
}

std::string cInterruptibleBlockingTCPSocket::getLocalInterface() const
{
    return getEndpointHostAddress(m_oSocket.local_endpoint());
}

uint16_t cInterruptibleBlockingTCPSocket::getLocalPort() const
{
    return getEndpointPort(m_oSocket.local_endpoint());
}

boost::asio::ip::tcp::endpoint cInterruptibleBlockingTCPSocket::getPeerEndpoint() const
{
    return m_oSocket.local_endpoint();
}

std::string cInterruptibleBlockingTCPSocket::getPeerAddress() const
{
    return getEndpointHostAddress(m_oSocket.remote_endpoint());
}

uint16_t cInterruptibleBlockingTCPSocket::getPeerPort() const
{
    return getEndpointPort(m_oSocket.remote_endpoint());
}

std::string cInterruptibleBlockingTCPSocket::getName() const
{
    return m_strName;
}

uint32_t cInterruptibleBlockingTCPSocket::getNBytesLastRead() const
{
    return m_u32NBytesLastRead;
}

uint32_t cInterruptibleBlockingTCPSocket::getNBytesLastWritten() const
{
    return m_u32NBytesLastWritten;
}

boost::system::error_code cInterruptibleBlockingTCPSocket::getLastWriteError() const
{
    return m_oLastWriteError;
}

boost::system::error_code cInterruptibleBlockingTCPSocket::getLastReadError() const
{
    return m_oLastReadError;
}

uint32_t cInterruptibleBlockingTCPSocket::getBytesAvailable() const
{
    return m_oSocket.available();
}

boost::asio::ip::tcp::socket* cInterruptibleBlockingTCPSocket::getBoostSocketPointer()
{
    return &m_oSocket;
}
