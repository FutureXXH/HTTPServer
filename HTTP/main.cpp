


#include <iostream>
#include "HttpServer.h"
#include "MyJson.h"

int main()
{
#ifndef ___WIN32_
	signal(SIGPIPE, SIG_IGN);
#endif // !___WIN32_

	SERVERPRINT_INFO << "正在开启服务器" << endl;
	HttpServer* HtServer = new HttpServer();
	HtServer->InitMysqlInfo("127.0.0.1", 3306, "test1", "123456", "testaccount");
	HtServer->RunServer();



	return 0;
}