//System includes

//Library includes:
#ifndef Q_MOC_RUN //Qt's MOC and Boost have some issues don't let MOC process boost headers
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#endif

//Local includes

class cInterruptibleBlockUDPSocket
{
private:
    //The serial port and io service for the port   
    boost::asio::io_service         m_oIOService;
    boost::asio::ip::udp::socket    m_oSocket;
    boost::asio::ip::udp::endpoint  m_oBoundLocalEndPoint;
    boost::asio::ip::udp::endpoint  m_oConnectedRemoteEndPoint;

    boost::asio::ip::udp::resolver  m_oResolver;

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
    cInterruptibleBlockUDPSocket(const std::string &strName = "");
    cInterruptibleBlockUDPSocket(const std::string &strRemoteAddress, uint16_t u16RemotePort, const std::string &strName = "");

    void                            openAndBindSocket(std::string strLocalAddress, uint16_t u16LocalPort);
    void                            openAndConnectSocket(std::string strRemoteAddress, uint16_t u16RemotePort);
    void                            close();

    bool                            send(char *cpBuffer, uint32_t u32NBytes, uint32_t u32Timeout_ms = 0);
    bool                            sendTo(char *cpBuffer, uint32_t u32NBytes, const std::string &strRemoteHost, uint16_t u16RemotePort, uint32_t u32Timeout_ms = 0);
    bool                            sendTo(char *cpBuffer, uint32_t u32NBytes, const boost::asio::ip::udp::endpoint &oRemoteEndPoint, uint32_t u32Timeout_ms = 0);

    bool                            receive(char *cpBuffer, uint32_t u32NBytes, uint32_t u32Timeout_ms = 0);
    bool                            receiveFrom(char *cpBuffer, uint32_t u32NBytes, std::string &strRemoteHost, uint16_t &u16RemotePort, uint32_t u32Timeout_ms = 0);
    bool                            receiveFrom(char *cpBuffer, uint32_t u32NBytes, boost::asio::ip::udp::endpoint &oRemoteEndPoint, uint32_t u32Timeout_ms = 0);

    void                            cancelCurrrentOperations();

    //Some utility functions
    boost::asio::ip::udp::endpoint  createEndPoint(std::string strHostAddress, uint16_t u16Port);
    std::string                     getEndPointHostAddress(boost::asio::ip::udp::endpoint oEndPoint);
    uint16_t                        getEndPointPort(boost::asio::ip::udp::endpoint oEndPoint);

    //Some accessors
    boost::asio::ip::udp::endpoint  getBoundLocalEndpoint();
    std::string                     getBoundLocalAddress();
    uint16_t                        getBoundLocalPort();

    boost::asio::ip::udp::endpoint  getConnectedRemoteEndPoint();
    std::string                     getConnectedRemoteAddress();
    uint16_t                        getConnectedRemotePort();

    std::string                     getName();

    uint32_t                        getNBytesLastTransferred();
    boost::system::error_code       getLastError();

    //Pass through some boost socket functionality:
    uint32_t                        getBytesAvailable();

};
