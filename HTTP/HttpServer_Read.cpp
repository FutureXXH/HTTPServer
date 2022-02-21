#include "HttpServer.h"




int HttpServer::analyData(Socket socketfd, HTTP_BASE* quest, HTTP_BASE* reponse)
{
	if (quest->state >= R_OVER)return 0;
	if (quest->state == R_HEAD) 
	{
		if ((quest->method != "POST" && quest->method != "PUT") && (quest->Content_length <= 0 || quest->Content_length > MAX_POST_LENGTH))
		{
			quest->state = R_ERROR;
			reponse->SetResponseLine(403, "Failed");
			writeData(quest, reponse, "err", 3);
			return -1;
		}
		//已经处理了行 数据头 处理消息数据
		readBody(socketfd, quest, reponse);
		return 0;
	}
	if (quest->state != R_FREE)return 0;

	int len = quest->pos_tail - quest->pos_head;
	quest->temp_str.clear();
	quest->temp_str.assign(quest->buf+quest->pos_head,len);



	int pos = quest->temp_str.find("\r\n\r\n");
	if (pos < 0)
	{
		if(quest->method != "PUT")
			if (len >= MAX_PACKAGE_LENGTH)
			{
				quest->state == R_ERROR;
				return -1;
			}
		return 0;
	}
	len = pos + 4;
	quest->temp_str.clear();
	quest->temp_str.assign(quest->buf+quest->pos_head, len);
	vector<string> arr = split(quest->temp_str, "\r\n", false);

	//请求行
	vector<string> line = split(arr[0], " ", true);
	quest->SetRequestLine(line);

	//头
	for (int i = 1; i < arr.size() - 1; i++)
	{
		vector<string> head = split2(arr[i], ":");
		if (head.size() == 2)
		{
			quest->SetHeader(head[0], head[1]);
			if (head[0] == "Content-Length")
			{
				quest->SetContentLength(head[1]);
			}
			else if (head[0] == "Content-Type")
			{
				vector<string> temp = split(head[1], ";");
				if (temp.size() > 0) quest->SetContentType(temp[0]);
			}
			else if (head[0] == "Connection")
			{
				quest->SetConnection(head[1]);
			}
		}

	}

	quest->pos_head += pos + 4;
#ifdef _DEBUG_INFOPRINT
	//===============调试输出=====================
	SERVERPRINT_INFO << socketfd << " " << quest->threadID << endl
		<< quest->method << " " << quest->url << " " << quest->version << endl;
		for (auto it = quest->head.begin(); it != quest->head.end(); it++)
		{
			SERVERPRINT_INFO << it->first << " : " << it->second << endl;
		}
	SERVERPRINT_INFO << endl;
	//===================================
#endif

	if (quest->method != "POST" && quest->method != "PUT")
	{
		quest->state = R_OVER;
		quest->temp_str.clear();
		bool isExist = read_Quest(quest->url, quest->temp_str);
		if (isExist)
		{
			reponse->SetResponseLine(200, "OK");
			writeData(quest, reponse, quest->temp_str.c_str(), quest->temp_str.size());
		}
		else
		{
			reponse->SetResponseLine(404, "Failed");
			writeData(quest, reponse, "NoExist", 7);
		}

		//获取数据

	

		return 0;
	}



	quest->state = R_HEAD;
	if (quest->Content_length <= 0 || quest->Content_length > MAX_POST_LENGTH)
	{
		quest->state = R_ERROR;
		reponse->SetResponseLine(402, "Failed");
		writeData(quest, reponse, "err", 3);
		return -1;
	}

	//读取消息体 数据
	readBody(socketfd, quest, reponse);



	return 0;
}

int HttpServer::readBody(Socket socketfd, HTTP_BASE* quest, HTTP_BASE* reponse)
{
	int len = quest->pos_tail - quest->pos_head;
	if (len < quest->Content_length)return 0; //还没有接收完

	//put只作为上传 
	if (quest->method == "PUT")
	{
		upload(quest, reponse);
		return 0; 
	}
	//判断数据类型
	if (quest->Content_Type == "application/protobuf")
	{

		return 0;
	}
	else if (quest->Content_Type == "application/binary")
	{
		binaryCommand(quest, reponse);
		return 0;
	}
	else if (quest->Content_Type == "application/json")
	{
		jsonCommand(quest, reponse);
		return 0;
	}

	string body(quest->buf, quest->pos_head, quest->Content_length);
	quest->pos_head += quest->Content_length;
	quest->state = R_OVER;

	//SERVERPRINT_INFO << body << endl;

	//响应消息数据

	reponse->SetResponseLine(200, "OK");
	writeData(quest, reponse, "ok", 2);


	return 0;
}

void HttpServer::binaryCommand(HTTP_BASE* quest, HTTP_BASE* reponse)
{
	if (quest->Content_length != sizeof(Test_1000))
	{
		quest->state = R_OVER;
		reponse->SetResponseLine(404, "Failed");
		writeData(quest, reponse, "err", 3);
	}

	Test_1000* d = new Test_1000();
	memset(d, 0, sizeof(Test_1000));
	memcpy(d, quest->buf + quest->pos_head, sizeof(Test_1000));
	quest->pos_head += quest->Content_length;
	quest->state = R_OVER;

	SERVERPRINT_INFO << d->cmd << " " << d->str << endl;
	reponse->SetResponseLine(200, "OK");
	writeData(quest, reponse, "ok", 2);
	delete d;
	
}

void HttpServer::jsonCommand(HTTP_BASE* quest, HTTP_BASE* reponse)
{
	quest->temp_str.clear();
	quest->temp_str.assign(quest->buf + quest->pos_head, quest->Content_length);
	quest->pos_head += quest->Content_length;
	quest->state = R_OVER;
	auto json = MyJson::ParseJson(quest->temp_str);
	for (auto it = json.begin(); it != json.end(); it++)
	{
		SERVERPRINT_INFO << it->first << "  " << it->second << endl;
	}
	reponse->SetResponseLine(200, "OK");
	writeData(quest, reponse, "ok", 2);


}





int HttpServer::recvSocket(Socket socketfd, HTTP_BASE* quest)
{
	memset(quest->tempBuf, 0, MAX_ONES_BUF);

	int recvBytes = recv(socketfd, quest->tempBuf, MAX_ONES_BUF, 0);

	if (recvBytes > 0)
	{
		if (quest->pos_head == quest->pos_tail)
		{
			quest->pos_tail = 0;
			quest->pos_head = 0;
		}
		if (quest->pos_tail + recvBytes >= MAX_BUF)return -1;
		memcpy(quest->buf + quest->pos_tail, quest->tempBuf, recvBytes);
		quest->pos_tail += recvBytes;
		return recvBytes;
	}

#ifdef ___WIN32_
	if (recvBytes < 0)
	{
		int err = WSAGetLastError();
		if (err == WSAEINTR) return 0;//被中断
		else if (err == WSAEWOULDBLOCK) return 0; //稍后再试
		else return -1;
	}
	else if (recvBytes == 0)
	{
		return -2;
	}
#else

	if (recvBytes < 0)
	{
		if (errno == EINTR) return 0;
		else if (errno == EAGAIN) return 0;
		else return -1;
}
	else if (recvBytes == 0)
	{
		return -2;
	}

#endif // ___WIN32_



	return 0;
}