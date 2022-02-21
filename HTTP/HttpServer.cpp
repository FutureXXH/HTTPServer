#include "HttpServer.h"



HttpServer::HttpServer()
{

}

HttpServer::~HttpServer()
{
#ifdef ___WIN32_
	if (Listenfd != INVALID_SOCKET)
	{
		closesocket(Listenfd);
		Listenfd = INVALID_SOCKET;

	}
	WSACleanup();
#else
	close(Listenfd);
#endif // ___WIN32_

}

int select_isRead(Socket socket, int s, int u)
{
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(socket, &fds);

	timeval tv;
	tv.tv_sec = s;
	tv.tv_usec = u;

	while (true)
	{
		int err = select(socket + 1, &fds, NULL, NULL, &tv);
		if (err < 0 && errno == EINTR)continue;
		return err;
	}
	return 0;
}


void HttpServer::RunServer()
{
	initPath();
	ConnectCount = 0;
	//初始化socket
   int err = InitSocket();
   if (err < 0)
   {
	   SERVERPRINT_INFO << "InitSocket err:" << err << endl;
   }
	//初始化请求响应数据
	for (int i = 0; i < MAX_THREAD_COUNT; i++)
	{
		Request[i] = new HTTP_BASE();
		Response[i] = new HTTP_BASE();
		Request[i]->Reset();
		Response[i]->Reset();
		Mysql[i] = new db::MysqlConnector();
	}
	//初始化mysql
	//初始化运行线程
	runThread();
	//运行主线程监听连接
	RunAccept();


}

bool SetNonblockingSocket(Socket socket)
{
#ifdef ___WIN32_
	unsigned long ul = 1;
	int err = ioctlsocket(socket, FIONBIO, (unsigned long*)&ul);
	if (err == SOCKET_ERROR)return false;
	return true;
#else
	int f = fcntl(socket, F_GETFL);
	if (f < 0)return false;
	f |= O_NONBLOCK;
	if (fcntl(socket, F_SETFL, flags) < 0)
		return false;
	return true;
#endif // ___WIN32_
}

int HttpServer::InitSocket()
{
#ifdef ___WIN32_
	WSADATA wsData;
	int errcode = WSAStartup(MAKEWORD(2, 2), &wsData);
	if (errcode != 0)return -1;
#endif // ___WIN32_

	Listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (Listenfd < 0)return -2;

	//关闭ngle
	int value = 1;
	setsockopt(Listenfd, IPPROTO_TCP, TCP_NODELAY, (char*)&value, sizeof(value));

	//设置非阻塞
	SetNonblockingSocket(Listenfd);

	//设置wait time
	int flag = 1;
	int res = setsockopt(Listenfd, SOL_SOCKET, SO_REUSEADDR, (char*)&flag, sizeof(int));

	sockaddr_in ServerAddr;
	memset(&ServerAddr, 0, sizeof(sockaddr_in));
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(8080);
#if ___WIN32_
	ServerAddr.sin_addr.S_un.S_addr = INADDR_ANY;
#else
	ServerAddr.sin_addr.s_addr = INADDR_ANY;
#endif // ___WIN32_
	int err = bind(Listenfd, (sockaddr*)&ServerAddr, sizeof(ServerAddr));
	if (err < 0)return -3;


	err = listen(Listenfd, SOMAXCONN);
	if (err < 0)return -4;


	return 0;
}



void HttpServer::runThread()
{
	for (int i = 0; i < MAX_THREAD_COUNT; i++)
	{
	
		Thread[i].reset(new thread(HttpServer::run, this, i));
		Thread[i]->detach();
	}
}



void HttpServer::runSocket(Socket socketfd, int id)
{
#ifdef ___WIN32_
	//解决无法shutdown问题
	setsockopt(socketfd, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&(Listenfd), sizeof(Listenfd));
#endif // ___WIN32_
	SetNonblockingSocket(socketfd);

	int value = 1;
	setsockopt(socketfd, IPPROTO_TCP, TCP_NODELAY, (char*)&value, sizeof(value));

	auto quest = Request[id];
	auto reponse = Response[id];

	quest->Reset();
	reponse->Reset();
	quest->threadID = id;
	reponse->threadID = id;
	//steady_clock 是单调的时钟
	auto start = chrono::steady_clock::now();
	while (true)
	{
		int err = select_isRead(socketfd, 0, 2000);
		if (err < 0)
		{
			SERVERPRINT_INFO << "select错误" << socketfd <<" " << err  <<" " << WSAGetLastError() << endl;
			break;
		}
		//接收数据
		if (err > 0)
		{
			err = recvSocket(socketfd, quest);
			if (err < 0)
			{
				SERVERPRINT_INFO << "接收时错误" << err << endl;
				break;
			}
		}
		//分析数据
		if (quest->state <= R_HEAD)
		{
			analyData(socketfd, quest, reponse);
		}
		//响应发包
		err = sendSocket(socketfd, reponse, id);
		if (err < 0)
		{
			
			break;
		}
		//是否处理完毕
		if (reponse->state == S_OVER)
		{

			reponse->state = S_FREE;
			if (quest->state == R_ERROR)
			{
				SERVERPRINT_INFO << "处理请求错误" << socketfd << endl;
				break;
			}
			else if (quest->Connection == "close")
			{
				break;
			}
			if (quest->putState != P_FREE)//上传完毕
			{
				break;
			}

			//处理一条请求完毕 初始化请求数据 等待下一个请求
			quest->Init();
		}

		//如果是keeplive情况

		if (quest->putState == P_FREE)
		{
			auto cur = chrono::steady_clock::now();
			auto dur = chrono::duration_cast<chrono::milliseconds>(cur - start);
			if (dur.count() > MAX_KEEP_ALIVE)
			{
				break;
			}
		}
		else
		{
			auto cur = chrono::steady_clock::now();
			auto dur = chrono::duration_cast<chrono::seconds>(cur - start);
			//上传下载 超时设置
			if (dur.count() > 30)
			{
				break;
			}
		}

	}

	ConnectCount--;

	SERVERPRINT_INFO << "关闭socket" << socketfd << " 当前连接:" << ConnectCount << "|" << Socketfds.size() << endl;

#ifdef ___WIN32_
	shutdown(socketfd, SD_BOTH);
	closesocket(socketfd);
#else
	shutdown(socketfd, SHUT_RDWR);
	close(socketfd);

#endif // ___WIN32_

}


void HttpServer::RunAccept()
{
	while (true)
	{
		int value = select_isRead(Listenfd, 0, 5000);
		if (value == 0)continue;
		socklen_t clen = sizeof(sockaddr);
		sockaddr_in clientAddr;

		int socketfd = accept(Listenfd, (sockaddr*)&clientAddr, &clen);
		if (socketfd < 0)
		{
			if (errno == ENFILE)continue;
			SERVERPRINT_INFO << "连接时 socket 错误" << endl;
			continue;
		}

		{
			unique_lock<mutex> lock(Mutex);
			Socketfds.emplace_back(socketfd);
		}
		ConnectCount++;
		SERVERPRINT_INFO << "连接数" << ConnectCount << " 队列: " << Socketfds.size() << endl;

		//通知工作线程
		Condition.notify_one();

	}
}

int HttpServer::sendSocket(Socket socketfd, HTTP_BASE* reponse, int id)
{
	if (reponse->state != S_SENDING)return 0;
	int len = reponse->pos_tail - reponse->pos_head;
	if (len <= 0)return 0;

	int sendBytes = send(socketfd, reponse->buf + reponse->pos_head, len, 0);

	reponse->pos_head += sendBytes;
	if (sendBytes > 0)
	{
		reponse->pos_head += sendBytes;
		if (reponse->pos_head == reponse->pos_tail)
		{
			reponse->Reset();
			reponse->state = S_OVER;
		}
		return 0;
	}
	SERVERPRINT_INFO << "发送错误: " << len  << "|" << sendBytes << endl;
#ifdef ___WIN32_
	if (sendBytes < 0)
	{
		int err = WSAGetLastError();
		if (err == WSAEINTR)return 0;
		else if (err == WSAEWOULDBLOCK)return 0;
		else return -1;
     }
	else if (sendBytes == 0)
	{
		return -2;
	}
#else
	if (sendBytes < 0)
	{
		if (err == EINTR)return 0;
		else if (err == EAGAIN)return 0;
		else return -1;
	}
	else if (sendBytes == 0)
	{
		return -2;
	}

#endif // __WIN32_


	return 0;
}
void HttpServer::InitMysqlInfo(const char* setIP, int Port, const char* username, const char* password, const char* dbname)
{
	sprintf(mysqlIP, setIP);
	mysqlPort = Port;
	sprintf(mysqlUsername, username);
	sprintf(mysqlPassword, password);
	sprintf(Dbname, dbname);

}



void HttpServer::run(HttpServer* s, int id)
{
	Socket socketfd = -1;
	

	bool isconnectmysql = s->Mysql[id]->ConnectMySql(s->mysqlIP,s->mysqlUsername, s->mysqlPassword, s->Dbname, s->mysqlPort);
	if (!isconnectmysql)
	{
		SERVERPRINT_INFO << "线程id:" << id << "连接mysql失败" << s->Mysql[id]->GetErrorStr() << endl;
		return;
	}
	SERVERPRINT_INFO << "线程id:" << id << "连接mysql成功" << endl;
	SERVERPRINT_INFO << "工作线程开启" << id << endl;
   while (true)
	{
		{
			unique_lock<mutex> lock(s->Mutex);
			while (s->Socketfds.empty())//防止虚假唤醒
			{
				s->Condition.wait(lock);
			}
			

			socketfd = s->Socketfds.front();
			s->Socketfds.pop_front();
			//SERVERPRINT_INFO << "连接数" << s->ConnectCount << " 队列: " << s->Socketfds.size() << endl;
		}

		//进行处理
		SERVERPRINT_INFO << "处理数据 " << endl;
		s->runSocket(socketfd, id);

	}
}
