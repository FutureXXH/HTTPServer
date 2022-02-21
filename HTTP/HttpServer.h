#ifndef __HTTPSERVER_H
#define __HTTPSERVER_H
#include "IDefine.h"
#include <atomic>
#include <chrono>
#include "MyJson.h"
#include "MySQLConnector.h"
using namespace std;
class HttpServer
{
  
private:
	atomic<int> ConnectCount;
	Socket Listenfd;
	mutex Mutex;
	mutex ConnectMutex;
	list<Socket> Socketfds;
	condition_variable Condition;
	shared_ptr<thread> Thread[MAX_THREAD_COUNT];

	HTTP_BASE* Request[MAX_THREAD_COUNT];
	HTTP_BASE* Response[MAX_THREAD_COUNT];


	db::MysqlConnector* Mysql[MAX_THREAD_COUNT];

	//=================
	char mysqlIP[20];
	int mysqlPort;
	char mysqlUsername[20];
	char mysqlPassword[20];
	char Dbname[20];
	
	//==================

	int analyData(Socket socketfd, HTTP_BASE* quest, HTTP_BASE* reponse);
	int readBody(Socket socketfd, HTTP_BASE* quest, HTTP_BASE* reponse);
	void binaryCommand(HTTP_BASE* quest, HTTP_BASE* reponse);
	void jsonCommand(HTTP_BASE* quest, HTTP_BASE* reponse);
	void upload(HTTP_BASE* quest, HTTP_BASE* reponse);
public:
	void InitMysqlInfo(const char* setIP, int Port, const char* username, const char* password, const char* dbname);
	HttpServer();
	virtual ~HttpServer();
	void RunServer();
	int InitSocket();

	void runThread();

	int recvSocket(Socket socketfd, HTTP_BASE* quest);
	
	void runSocket(Socket socketfd, int id);
	void RunAccept();
	int sendSocket(Socket socketfd, HTTP_BASE* reponse, int id);
	void  writeData(HTTP_BASE* quest, HTTP_BASE* reponse, const char* body, int size);

	static void run(HttpServer* s, int id);

};

#endif // !__HTTPSERVER_H
