//#program once

#ifndef  ___SERVER_CHAT_H
#define  ___SERVER_CHAT_H

#include <unistd.h>
#include <atomic>
#include <mutex>
#include <thread>
#include <set>
#include <map>
#include <memory>
#include "net/Socket.h"
#include "net/Connection.h"
#include "net/Application.h"
#include "util/Logger.h"
#include "ImPduBase.h"
#include "ConfigFileReader.h"


//this class targets to ServerConnnection
//it must be single instance in an application
//>end
using namespace ananas;

class  ServerChat{



public:
   typedef  std::unique_ptr<CImPdu>  message_type;
   ServerChat();
   void Start(int argc,char **argv);
   SocketAddr &getSocket();
   void       SetSocket(SocketAddr &addr);
      

private:
  std::mutex             m_mutex;
  std::set<Connection *> _Connections;
  SocketAddr             m_socketaddr;
  int                    m_threadNum;
  std::set<Connection*>  m_other_Connections;
  

//handler message
private:
  void HandleLoginReq(Connection *,message_type *);

private:
  //static Application app;
  bool _OnInit(int ,char *[]);
  void OnNewConnection(ananas::Connection * conn);
  void OnClientNewConnection(ananas::Connection * conn);
  PacketLen_t  OnMessage(ananas::Connection * conn, const char * data, ananas :: PacketLen_t len);
  void  OnDisConnect(ananas::Connection * conn);
  void OnClientConnFail(ananas::EventLoop* loop, const ananas::SocketAddr& peer);

  

  
  	

};





















#endif 








