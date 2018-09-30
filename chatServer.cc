#include "chatServer.h"
#include "ImPduBase.h"
#include "IM.BaseDefine.pb.h"
#include "IM.Other.pb.h"
using namespace IM::BaseDefine;

extern std::shared_ptr<ananas::Logger> logger;


ServerChat::ServerChat()
{}

bool ServerChat::_OnInit(int argc,char *argv[])
{   

	auto & app=ananas::Application::Instance();
    app.SetNumOfWorker(m_threadNum);
	app.Listen(m_socketaddr,std::bind(&ServerChat::OnNewConnection,this,std::placeholders::_1));
	INF(logger)<<"start server success.....";
    return true;
}

void ServerChat::Start(SocketAddr & addr, int threadNum,int argc,char *argv[])
{
	m_socketaddr=addr;
	m_threadNum=threadNum;
	auto & app=ananas::Application::Instance();
   //app.SetOnInit(std::bind(&ServerChat::_OnInit,this,argc,argv));
    app.SetNumOfWorker(m_threadNum);
	app.Listen(m_socketaddr,std::bind(&ServerChat::OnNewConnection,this,std::placeholders::_1));
	INF(logger)<<m_socketaddr.ToString()<<": start server success.....";
	app.Run(argc,argv);
	
}

void ServerChat::OnNewConnection(ananas::Connection * conn)
{  
    conn->SetOnMessage(std::bind(&ServerChat::OnMessage,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));
	conn->SetOnDisconnect(std::bind(& ServerChat::OnDisConnect,this,std::placeholders::_1));
	//safe-thread 
	
	std::lock_guard<std::mutex>guard(m_mutex);
    auto iter=_Connections.find(conn);
	if(iter==std::end(_Connections))
	{
	     _Connections.insert(conn);
	    INF(logger)<<"new client connection <"<<conn->Peer().ToString()<<">linked.... now total connection count "<<_Connections.size();	 
	}
	
}


PacketLen_t ServerChat::OnMessage(ananas::Connection * conn, const char * data, ananas :: PacketLen_t len)
{
      std::string req(data,len);
      //safe-thread
      //INF(logger)<<data;
      CImPdu *Ppdu=nullptr;
	  std::unique_ptr<CImPdu>Ppdu_ptr;
	  Ppdu_ptr=std::unique_ptr<CImPdu>(CImPdu::ReadPdu((uchar_t *)data,(uint32_t)len));
	  
      if ((Ppdu=Ppdu_ptr.get()))
      {
	        switch(Ppdu->GetCommandId())
	        {

			    case CID_OTHER_HEARTBEAT:{
		            INF(logger)<<"heartbeat......";
					
		            break;
			    	}
				default:
				    INF(logger)<<"default......";	
					break;
			   //case  
	        }
	  
	  
      }else{
            ERR(logger)<<"...ERR......"<<Ppdu;
	  }
	  
	  
      
	 	
     return len;
}


void  ServerChat::OnDisConnect(ananas::Connection * conn)
{

	INF(logger)<<"client Connection Close"<<conn->Peer().ToString();
	std::lock_guard<std::mutex>guard(m_mutex);
	_Connections.erase(conn);
  
}

void ServerChat::SetSocket(SocketAddr & addr)
{
    m_socketaddr=addr;
}

SocketAddr & ServerChat::getSocket()
{
   return m_socketaddr;
}






