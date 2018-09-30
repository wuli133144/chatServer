#include <iostream>
#include "chatServer.h"
#include "ImPduBase.h"
#include "IM.BaseDefine.pb.h"
#include "IM.Other.pb.h"
#include "IM.Login.pb.h"

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

void ServerChat::Start(int argc,char *argv[])
{   

   try{
	    CConfigFileReader _config("config.txt");
		char *pserveraddr1=_config.GetConfigName("ip1_server");
        int   port1        =std::atoi(_config.GetConfigName("port1"));
		std::cout<<pserveraddr1 <<" : "<<port1<<std::endl;
		char *pserveraddr2=_config.GetConfigName("ip2_server");
        int   port2        =std::atoi(_config.GetConfigName("port2"));
		
		m_socketaddr=ananas::SocketAddr(pserveraddr1,port1);
		m_threadNum=std::atoi(_config.GetConfigName("NumThread"));

       
		//std::cout<<pserveraddr1 <<" : "<<port1<<std::endl<<pserveraddr2<<" : "<<port2<<std::endl<<" threadnum "<<m_threadNum<<std::endl;
		#if 1
		auto & app=ananas::Application::Instance();
	   //app.SetOnInit(std::bind(&ServerChat::_OnInit,this,argc,argv));
	    app.SetNumOfWorker(m_threadNum);
		app.Listen(m_socketaddr,std::bind(&ServerChat::OnNewConnection,this,std::placeholders::_1));
		app.Connect(pserveraddr2,port2, 
			std::bind(&ServerChat::OnClientNewConnection,this,std::placeholders::_1),
			std::bind(&ServerChat::OnClientConnFail,this,std::placeholders::_1,std::placeholders::_2), 
			ananas::DurationMs(3000));
		INF(logger)<<"Listening "<<m_socketaddr.ToString()<<": start server success.....";
		
		
		app.Run(argc,argv);
		#endif 
		
   	}catch(...)
   	{
   	     INF(logger)<<"load configure file failed!";
		 return;
   	}

	return;
	
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

//connect other server device failed
void  ServerChat::OnClientConnFail(ananas::EventLoop* loop, const ananas::SocketAddr& peer)
{          
    INF(logger) << "OnClientConnFail " << peer.GetPort();

    // reconnect
    loop->ScheduleAfter(std::chrono::seconds(2),
    [=]() {
        loop->Connect(peer,
                     std::bind(&ServerChat::OnClientNewConnection,this,std::placeholders::_1),
					 std::bind(&ServerChat::OnClientConnFail,this,std::placeholders::_1,std::placeholders::_2), 
					 ananas::DurationMs(3000));
    });

   
}
//new connection
void ServerChat::OnClientNewConnection(ananas::Connection * conn)
{
    conn->SetOnConnect([=](ananas::Connection * conn){

	        assert(conn->GetLoop()->IsInSameLoop());
	        if(m_other_Connections.find(conn) == std::end(m_other_Connections))
	        {
	          // 
	          m_other_Connections.insert(conn);
	        }

			for(auto &i:m_other_Connections)
			{     
			      //test request message
			      std::string msg="xxxxxxx";
			      i->SendPacket(msg.data(),msg.size());
		          INF(logger)<<"test request"<<msg.data();		   
			}
                           

		});

   conn->SetOnMessage([&](ananas::Connection * conn, const char * data, ananas :: PacketLen_t len){
   	           //
   	           return len;
   	           
   });

  
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
				case CID_LOGIN_REQ_USERLOGIN:
					HandleLoginReq(conn,&Ppdu_ptr);
					break;
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

void ServerChat::HandleLoginReq(Connection * conn,message_type * ptr_ppdu)
{
        INF(logger)<<"login request";
		IM::Login::IMLoginReq msg;
		if(!msg.ParseFromArray((*ptr_ppdu)->GetBodyData(),(*ptr_ppdu)->GetBodyLength()))
		{
		       INF(logger)<<"login request pb parse error!";   
			   return;
		}

		INF(logger)<<"user_name "<<msg.user_name()<<"password "<<msg.password();

		//handle db business logic
		//something()

		if(_Connections.empty())
		{
		     for(auto &i:_Connections)
		     {
		       if(i != conn)
		       	{
		       	   i->SendPacket((*ptr_ppdu)->GetBuffer(),(*ptr_ppdu)->GetLength());
		           INF(logger)<<"send login request";		   
		       	}
		     }
		}

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






