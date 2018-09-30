#include <unistd.h>
#include <iostream>
#include <string>
#include <assert.h>
#include <thread>
#include <mutex>

#include "net/Connection.h"
#include "net/EventLoop.h"
#include "net/Application.h"
#include "util/Logger.h"
#include "util/TimeUtil.h"
#include "util/ConfigParser.h"
#include "ImPduBase.h"
#include "IM.BaseDefine.pb.h"
#include "IM.Other.pb.h"


ananas::Time start, end;
int nowCount = 0;
const int totalCount = 200 * 10000;

using namespace ananas;
using namespace std;
using namespace IM::BaseDefine;

std::shared_ptr<ananas::Logger> logger;
std::once_flag                  m_flag;

ConfigParser                    m_config;

std::unique_ptr<Connection> ptr_connection;

ananas::PacketLen_t OnMessage(ananas::Connection* conn, const char* data, size_t len) {
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
    //std::string rsp(data, len);
	//INF(logger)<<rsp;
    //conn->SendPacket(rsp.data(), rsp.size());
    
	std::cout<<"OnMessage pid"<<std::this_thread::get_id()<<std::endl;

	

	
    
    return len;
}

void OnWriteComplete(ananas::Connection* conn) {
     INF(logger) << "OnWriteComplete for " << conn->Identifier();
}

void OnConnect(ananas::Connection* conn) {
    INF(logger) << "OnConnect " << conn->Identifier();

    std::string tmp = "abcdefghijklmnopqrstuvwxyz[request]";
    //std::string msg = tmp ;//+ tmp + tmp + tmp;
    //input


	std::cout<<"OnConnect pid"<<std::this_thread::get_id()<<std::endl;
   

	
    //conn->SendPacket(Pdu.GetBuffer(),Pdu.GetLength());	
    //auto &loop=*(conn->GetLoop());
	
	/*loop.Execute([&](){
		std::this_thread::sleep_for(std::chrono::seconds(3));
	    conn->SendPacket(Pdu.GetBuffer(),Pdu.GetLength());	   
	});
	*/
	
	//auto &loop1=*(EventLoop::GetCurrentEventLoop());
	//loop1.ScheduleAfterWithRepeat<3>(std::chrono::seconds(1),[&](){
		//std::this_thread::sleep_for(std::chrono::seconds(3));
	//    conn->SendPacket(Pdu.GetBuffer(),Pdu.GetLength());	   
	//});

    
   ptr_connection=std::unique_ptr<Connection>(conn);


	auto &loop=*(conn->GetLoop());
	//std::shared_ptr<Connection>connection_ptr(conn);
	//std::cout<<"onConnect : use_count"<<ptr_connection.use_count()<<std::endl;
	loop.ScheduleAfterWithRepeat<kForever>(std::chrono::seconds(3),[](){
		
			IM::Other::IMHeartBeat msg;
			CImPdu Pdu;
			Pdu.SetPBMsg(&msg);
			Pdu.SetServiceId(SID_OTHER);
			Pdu.SetCommandId(CID_OTHER_HEARTBEAT);
		   std::cout<<"connected thread_id:"<<std::this_thread::get_id()<<"linked connection count:"<<std::endl;
		   //conn->GetLoop()==loop
		    ptr_connection.get()->SendPacket(Pdu.GetBuffer(),Pdu.GetLength());	
	});

    
    
}

void OnDisConnect(ananas::Connection* conn) {
    INF(logger) << "OnDisConnect " << conn->Identifier();
	
}

void OnNewConnection(ananas::Connection* conn) {
    conn->SetOnConnect(OnConnect);
    conn->SetOnMessage(OnMessage);
    conn->SetOnDisconnect(OnDisConnect);
    conn->SetOnWriteComplete(OnWriteComplete);
	//conn->SetBatchSend(bool batch)
	std::cout<<"OnNewConnection pid:"<<std::this_thread::get_id()<<std::endl;

	
}


void OnConnFail(ananas::EventLoop* loop, const ananas::SocketAddr& peer) {
    INF(logger) << "OnConnFail " << peer.GetPort();

    // reconnect
    loop->ScheduleAfter(std::chrono::seconds(2),
    [=]() {
        loop->Connect(peer,
                      OnNewConnection,
                      OnConnFail,
                      ananas::DurationMs(3000));
    });
}

void SanityCheck(int& threads) {
    if (threads < 1)
        threads = 1;
    else if (threads > 100)
        threads = 100;
}

int main(int ac, char* av[]) {
      
    std::call_once(m_flag,[&](){
		try{
			ananas::LogManager::Instance().Start();
			logger = ananas::LogManager::Instance().CreateLog(logALL, logConsole);
		    //SignalHandler(int num)
			//m_config.Load("config.txt");
		}catch(...)
		{  
			ERR(logger)<<"open config file failed...";
			return -1;
		}
	});

    std::cout<<"main pid:"<<std::this_thread::get_id()<<std::endl;
    
    int threads = 1;
    if (ac > 1) {
        threads = std::stoi(av[1]);
        SanityCheck(threads);
    }

    const uint16_t port = 9986;
    const int kConns = threads;

    auto& app = ananas::Application::Instance();
    app.SetNumOfWorker(1);
	app.SetOnExit([&](){
		 ptr_connection.release();
		 ananas::LogManager::Instance().Stop();
	});

	auto &loop=*app.BaseLoop();

	loop.ScheduleAfterWithRepeat<kForever>(std::chrono::seconds(3),[&](){
		   std::cout<<"main thread test............thread_id:"<<std::this_thread::get_id()<<std::endl;
	});

	//auto &loop=*app.BaseLoop();
	
    //for (int i = 0; i < kConns; ++ i)
    std::string  strIpaddr;
	strIpaddr.reserve(64);
	int          nport=0;
    m_config.GetData<string>("IpAdress",strIpaddr);
	m_config.GetData<int>("port",nport);

	INF(logger)<<"ipaddr:"<<strIpaddr<<"  port:"<<nport;
	//m_config.Print();
    		
    app.Connect("127.0.0.1", port, OnNewConnection, OnConnFail, ananas::DurationMs(3000));
    // app.Connect()
    //auto &loop=*app.BaseLoop();
	//loop.ScheduleAfterWithRepeat<kForever>(std::chrono::seconds(3),[&](){
		 //send heartbeat
	//});
    start.Now();
    app.Run(ac, av);

    return 0;
}

