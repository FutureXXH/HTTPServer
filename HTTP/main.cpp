


#include <iostream>
#include "HttpServer.h"
#include "MyJson.h"
#include "IDefine.h"

int main()
{

	std::ios::sync_with_stdio(false);
#ifndef ___WIN32_
	signal(SIGPIPE, SIG_IGN);
#endif // !___WIN32_

	SERVERPRINT_INFO << "starting server" << endl;
	HttpServer* HtServer = new HttpServer();
	HtServer->InitMysqlInfo("127.0.0.1", 3306, "test1", "Xxh20010107", "testaccount");
	HtServer->RunServer();



	return 0;
}