
//System includes
#include <iostream>

//Library includes
#ifndef Q_MOC_RUN //Qt's MOC and Boost have some issues don't let MOC process boost headers
#include <boost/bind.hpp>
#include <boost/asio/placeholders.hpp>
#endif

//Local includes
#include "InterruptibleBlockingTCPAcceptor.h"

using namespace std;

cInterruptibleBlockingTCPAcceptor::cInterruptibleBlockingTCPAcceptor(const string &strName) :
    m_oAcceptor(m_oIOService),
    m_oTimer(m_oIOService),
    m_oResolver(m_oIOService),
    m_bError(true),
    m_strName(strName)
{

}

cInterruptibleBlockingTCPAcceptor::cInterruptibleBlockingTCPAcceptor(const string &strLocalInterface, uint16_t u16Port, const string &strName) :
    m_oAcceptor(m_oIOService),
    m_oTimer(m_oIOService),
    m_oResolver(m_oIOService),
    m_bError(true),
    m_strName(strName)
{
    openAndListen(strLocalInterface, u16Port);
}

void cInterruptibleBlockingTCPAcceptor::openAndListen(const string &strLocalInterface, uint16_t u16Port)
{
    m_oAcceptor.open(boost::asio::ip::tcp::v4()); //Use only IPv4
    m_oAcceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true)); //Set Listening socket to reuse address.
    m_oAcceptor.bind(boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(strLocalInterface), u16Port));
    m_oAcceptor.listen();
}

void cInterruptibleBlockingTCPAcceptor::close()
{
    //If the socket is open close it
    if(m_oAcceptor.is_open())
    {
        cout << "cInterruptibleBlockingTCPAcceptor::close(): Closing TCP Acceptor." << endl;
        m_oAcceptor.cancel();
        m_oAcceptor.close();
    }
}

bool cInterruptibleBlockingTCPAcceptor::isOpen()
{
    return m_oAcceptor.is_open();
}


bool cInterruptibleBlockingTCPAcceptor::accept(cInterruptibleBlockingTCPSocket &oSocket, string &strPeerAddress, uint32_t u32Timeout_ms)
{
    //Necessary after a timeout:
    m_oAcceptor.get_io_service().reset();
    boost::asio::ip::tcp::endpoint oPeerEndpoint;

    //Asynchronously accept socket connections
    m_oAcceptor.async_accept(*oSocket.getBoostSocketPointer(), oPeerEndpoint,
            boost::bind(&cInterruptibleBlockingTCPAcceptor::callback_complete,
                this,
                boost::asio::placeholders::error ) );

    // Setup a deadline time to implement our timeout.
    if(u32Timeout_ms)
    {
        m_oTimer.expires_from_now( boost::posix_time::milliseconds(u32Timeout_ms) );

        m_oTimer.async_wait( boost::bind(&cInterruptibleBlockingTCPAcceptor::callback_timeOut,
                    this, boost::asio::placeholders::error) );
    }

    // This will block until a new connection has been accepted
    // or until the it is cancelled.
    m_oAcceptor.get_io_service().run();

    if(m_bError)
        strPeerAddress = string("");
    else
        strPeerAddress = getEndpointHostAddress(oPeerEndpoint);

    return !m_bError;
}


bool cInterruptibleBlockingTCPAcceptor::accept(boost::shared_ptr<cInterruptibleBlockingTCPSocket> pSocket, string &strPeerAddress, uint32_t u32Timeout_ms)
{
    return accept(*pSocket.get(), strPeerAddress, u32Timeout_ms);
}

void cInterruptibleBlockingTCPAcceptor::callback_complete(const boost::system::error_code& oError)
{
    m_bError = true;
    if (boost::system::errc::success == oError)
    {
      m_bError = false;
    }
    //RE: m_bError = oError;
    m_oTimer.cancel();

    m_oLastError = oError;
}

void cInterruptibleBlockingTCPAcceptor::callback_timeOut(const boost::system::error_code& oError)
{
    if (oError)
    {
        m_oLastError = oError;
        return;
    }

    cout << "!!! Time out reached on socket acceptor \"" << m_strName << "\" (" << this << ")" << endl;

    m_oAcceptor.cancel();
}

void cInterruptibleBlockingTCPAcceptor::cancelCurrrentOperations()
{
    m_oTimer.cancel();
    m_oAcceptor.cancel();
}

boost::asio::ip::tcp::endpoint cInterruptibleBlockingTCPAcceptor::createEndpoint(string strHostAddress, uint16_t u16Port)
{
    stringstream oSS;
    oSS << u16Port;

    return *m_oResolver.resolve(boost::asio::ip::tcp::resolver::query(boost::asio::ip::tcp::v4(), strHostAddress, oSS.str()));
}

string cInterruptibleBlockingTCPAcceptor::getEndpointHostAddress(boost::asio::ip::tcp::endpoint oEndPoint)
{
    return oEndPoint.address().to_string();
}

uint16_t cInterruptibleBlockingTCPAcceptor::getEndpointPort(boost::asio::ip::tcp::endpoint oEndPoint)
{
    return oEndPoint.port();
}

boost::asio::ip::tcp::endpoint cInterruptibleBlockingTCPAcceptor::getLocalEndpoint()
{
    return m_oAcceptor.local_endpoint();
}

string cInterruptibleBlockingTCPAcceptor::getLocalInterface()
{
    return getEndpointHostAddress(m_oAcceptor.local_endpoint());
}

uint16_t cInterruptibleBlockingTCPAcceptor::getLocalPort()
{
    return getEndpointPort(m_oAcceptor.local_endpoint());
}

string cInterruptibleBlockingTCPAcceptor::getName()
{
    return m_strName;
}

boost::system::error_code cInterruptibleBlockingTCPAcceptor::getLastError()
{
    return m_oLastError;
}

