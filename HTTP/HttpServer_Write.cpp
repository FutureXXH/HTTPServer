#include "HttpServer.h"
#include <string>


void HttpServer::upload(HTTP_BASE* quest, HTTP_BASE* reponse)
{
	quest->putState = P_PUT;
	char* buf = new char[quest->Content_length];
	memset(buf, 0, quest->Content_length);
	memcpy(buf, quest->buf + quest->pos_head, quest->Content_length);

	quest->pos_head += quest->Content_length;
	quest->state = R_OVER;

	int err = write_File(quest->url, buf, quest->Content_length);

	delete[] buf;
	reponse->SetResponseLine(200, "OK");
	writeData(quest, reponse, "ok", 2);


}


string getReponseStr(HTTP_BASE* quest, HTTP_BASE* reponse)
{
	string  str;
	str += reponse->version + " " + to_string(reponse->status) + " " + reponse->describe + "\r\n";

	auto it = quest->head.begin();
	while (it != quest->head.end())
	{
		auto key = it->first;
		auto value = it->second;
		str += key + ":" + value + "\r\n";
		++it;
	}
	str += "\r\n";
	return str;

}



void HttpServer::writeData(HTTP_BASE* quest, HTTP_BASE* reponse, const char* body, int size)
{
	if (reponse->state != S_FREE)return;
	if (body == NULL)return;
	if (size <= 0 || size > MAX_ONES_BUF) return;


	quest->SetHeader("Content-Length", to_string(size));
	reponse->state = S_SENDING;

	string str = getReponseStr(quest, reponse);
#ifdef _DEBUG_INFOPRINT
	//===================调试信息输出============================
	SERVERPRINT_INFO << "Reponse : " << quest->threadID << endl
		<< str << body << endl;
	//================================================
#endif // _DEBUG_INFOPRINT



	//填充数据
	if (reponse->pos_tail + str.size() + size < MAX_BUF)
	{
		memcpy(reponse->buf + reponse->pos_tail, str.c_str(), str.size());
		reponse->pos_tail += str.size();
	
		memcpy(reponse->buf + reponse->pos_tail, body, size);
		reponse->pos_tail += size;
	}

}