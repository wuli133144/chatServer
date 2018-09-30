#include "chatServer.h"

using ananas::Connection;

std::shared_ptr<ananas::Logger> logger;
std::once_flag                  m_flag;
std::mutex                      mu;
std::map<int,const Connection *>Order_groups;


//route server area
//template like <userId,connection>
using RouteConnection=std::vector<Connection *>;
using DBConnection     =std::vector<Connection *>;
//>end 


#if 0
namespace ananas{

    class APP{
       public:
			 static APP & Instance(){ 
			    static APP app;	
			 	return app;}
             static Application & getApplication(){ return Application::Instance();}
             RouteConnection routeV;  	  
       private:
	   	     APP(){}	
	};
         

};

auto &app1=ananas::APP::Instance();





ananas::PacketLen_t OnMessage(ananas::Connection* conn, const char* data, ananas::PacketLen_t len) {
    // echo package
    //data.append("[wuyujie:<response>]");
    
    std::string rsp(data, len);
	//rsp.append("[wuyujie:<response>]");
	////receive request content
	INF(logger)<<rsp;
    //conn->SendPacket(rsp.data(), rsp.size());
    //send message to routeserver
    std::unique_lock<std::mutex>guard(mu);
	if (!app1.routeV.empty() )
	{
	    for(auto &i :app1.routeV)
	    {
	       i->SendPacket(rsp.data(), rsp.size());
	    }
	}
	guard.unlock();
    return len;
}

void OnNewConnection(ananas::Connection* conn) {
    using ananas::Connection;

	INF(logger) << conn->Peer().ToString();
	{
		std::unique_lock<std::mutex>guard(mu);
		if(Order_groups.find(conn->Identifier()) == Order_groups.end())
		{
		   Order_groups.insert({conn->Identifier(),conn});
		}
		guard.unlock();
	}
	
    conn->SetOnMessage(OnMessage);
    conn->SetOnDisconnect([](Connection* conn) {
        WRN(logger) << "OnDisConnect " << conn->Identifier();
		{
		std::unique_lock<std::mutex>guard(mu);
		if(Order_groups.find(conn->Identifier()) != Order_groups.end())
		{
		   Order_groups.erase(conn->Identifier());
		}
		guard.unlock();
  	    }
    });
}

//link other server
///////////////////////////////////////////////////
ananas::PacketLen_t OnMessageClient(ananas::Connection* conn, const char* data, size_t len) {
    /*++ nowCount;
    if (nowCount == totalCount) {
        end.Now();
        USR(logger) << "Done OnResponse avg " << (totalCount * 0.1f / (end - start)) << " W/s";
        ananas::Application::Instance().Exit();
        return 0;
    }
    if (nowCount % 100000 == 0) {
        end.Now();
        USR(logger) << "OnResponse avg " << (nowCount * 0.1f / (end - start)) << " W/s";
    }
    */
    // echo package
    std::string rsp(data, len);
	INF(logger)<<rsp;
    conn->SendPacket(rsp.data(), rsp.size());
    return len;
}

void OnWriteComplete(ananas::Connection* conn) {
     INF(logger) << "OnWriteComplete for " << conn->Identifier();
}


void OnConnect(ananas::Connection* conn) {
    INF(logger) << "OnConnect " << conn->Identifier();
	
	//auto routeV=app1.routeV;
	std::lock_guard<std::mutex>guard(mu);
	//auto routeVT=routeV.swap();
	{
		if(conn !=nullptr){
		   for(auto i : app1.routeV)
		   {
		       if(i == conn)
		       	{
		       	  return;
		       	}
		   }
	       app1.routeV.emplace_back(conn);
		}
	}
    std::string tmp = "abcdefghijklmnopqrstuvwxyz[request]";
    std::string msg = tmp ;//+ tmp + tmp + tmp;    
    conn->SendPacket(msg.data(), msg.size());
	
}

void OnDisConnect(ananas::Connection* conn) {
    INF(logger) << "OnDisConnect " << conn->Identifier();
}


void OnNewConnectionClient(ananas::Connection *conn)
{
    conn->SetOnConnect(OnConnect);
    conn->SetOnMessage(OnMessageClient);
    conn->SetOnDisconnect(OnDisConnect);
    conn->SetOnWriteComplete(OnWriteComplete);
}

void OnConnFailClient(ananas::EventLoop* loop, const ananas::SocketAddr& peer) {
    INF(logger) << "OnConnFail " << peer.GetPort();

    // reconnect
    loop->ScheduleAfter(std::chrono::seconds(2),
    [=]() {
        loop->Connect(peer,
                      OnNewConnectionClient,
                      OnConnFailClient,
                      ananas::DurationMs(3000));
    });
}



 

#endif
/////////////////////////////////////////////////////////////////////////////
int main(int ac, char* av[]) {
   
	std::call_once(m_flag,[&](){
	    ananas::LogManager::Instance().Start();
	    logger = ananas::LogManager::Instance().CreateLog(logALL, logConsole);
	});
	
    
    
    //auto& app = ananas::Application::Instance();
    #if 0	
	auto &app=ananas::APP::getApplication();
    app.SetNumOfWorker(workers);
	//auto loop=*app.BaseLoop();
    app.Connect("127.0.0.1", 9986, OnNewConnectionClient, OnConnFailClient, ananas::DurationMs(3000));
    app.Listen("127.0.0.1", 9987, OnNewConnection);
    app.Run(ac, av);
	#endif
    
	ServerChat server;
	server.Start(ac,av);
	ananas::LogManager::Instance().Stop();
    
    return 0;
}

