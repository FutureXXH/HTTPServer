#ifndef __IDefine_H
#define __IDefine_H

//#define _DEBUG_INFOPRINT

#define USEMYSQL


#ifdef ___WIN32_
#include<winsock2.h>
#include<WS2tcpip.h>
#include <MSWSock.h>
#pragma comment(lib,"ws2_32.lib")
using Socket = SOCKET;


#else
#define _atoi64(val)     strtoll(val, NULL, 10)  
#include <signal.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>
using Socket = int;



#endif // ___WIN32_

#include <thread>
#include <mutex>
#include <list>
#include <map>
#include <vector>
#include <condition_variable>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>

#ifndef S_ISREG
#define S_ISREG(m) (((m)&S_IFREG) == S_IFREG)
#endif // !S_ISREG




#define SERVERPRINT_INFO std::cout

#define MAX_EXE_LEN 200
#define MAX_KEEP_ALIVE 500
#define MAX_THREAD_COUNT 8
#define MAX_PACKAGE_LENGTH 1024
#define MAX_POST_LENGTH 1024*256

#define MAX_BUF 1024*1024*64
#define MAX_ONES_BUF 1024*1024*20




	extern std::string deleteString(std::string s, char c);
	extern std::vector<std::string> split(std::string str, std::string pattern, bool isadd = false);
	extern std::vector<std::string> split2(std::string str, std::string pattern);
	extern bool is_file(const std::string& path);
	extern void read_file(const std::string& path, std::string& out);
	extern bool read_Quest(const std::string& path, std::string& out);
	extern bool write_File(const std::string& filename, char* c, int len);
	extern void initPath();






	enum RECV_STATE
	{
		R_FREE = 0, //空闲状态
		R_HEAD = 1,//头已解析完毕 可能即将解析消息体
	    R_OVER = 2,//解析完毕
		R_ERROR = 3//出现错误

	};
	enum SEND_STATE
	{
		S_FREE = 0, // 空闲
		S_SENDING = 1,//正在发送
		S_OVER = 2 //发送完毕

	};
	enum CONNECT_STATE
	{
		C_FREE = 0,
		C_CONNECT = 1
	};
	enum PUTUPLOAD_STATE
	{
		P_FREE = 0,
		P_PUT = 1,
		P_LOAD = 2
	};
#pragma pack(push,packing)
#pragma pack(1)
	

	struct Test_1000
	{
		int cmd;
		int str[20];
	};

	//HTTP 响应请求结构体
	struct HTTP_BASE
	{
		int state;
		char buf[MAX_BUF];
		char tempBuf[MAX_ONES_BUF];
		std::string temp_str;
		int pos_head;
		int pos_tail;
		int threadID;
		std::string method;
		std::string url;
		std::string version;
		std::map<std::string, std::string> head;
		int Content_length;
		std::string Content_Type;
		std::string Connection;
		int putState;
		int status;
		std::string describe;

		inline void Reset()
		{
			memset(buf, 0, MAX_BUF);
			memset(tempBuf, 0, MAX_ONES_BUF);
			state = 0;
			status = -1;
			pos_head = 0;
			pos_tail = 0;
			method = "";
			url = "";
			version = "";
			Content_length = 0;
			Content_Type = "";
			Connection = "";
			putState = P_FREE;
			head.clear();
			threadID = 0;
		}
		inline void Init()
		{

			memset(tempBuf, 0, MAX_ONES_BUF);
			state = 0;
			status = -1;
			method = "";
			url = "";
			version = "";
			Content_length = 0;
			Content_Type = "";
			Connection = "";
			putState = P_FREE;
			head.clear();

		}

		inline void SetRequestLine(std::vector<std::string>& line)
		{
			
			if (line.size() != 3)return;
			method = line[0];
			url = line[1];
			version = line[2];
		
		}

		inline void SetHeader(std::string key, std::string value)
		{
			auto it = head.find(key);
			if (it != head.end())
			{
				head.erase(it);
			}
			head.insert(std::make_pair(key, value));
		}
		inline std::string GetHeader(std::string key)
		{
			auto it = head.find(key);
			if (it != head.end())
			{
				return it->second;
			}
			return "";
		}
		inline void SetContentLength(std::string value)
		{
			auto s = deleteString(value, ' ');
			int length = atoi(s.c_str());
			Content_length = length;
		}

		inline void SetContentType(std::string value)
		{
			auto s = deleteString(value, ' ');
			Content_Type = s;


		}


		inline void SetConnection(std::string value)
		{
			auto s = deleteString(value, ' ');
			Connection = s;
		}


		inline void SetResponseLine(int stat, std::string s)
		{
			status = stat;
			version = "HTTP/1.1";
			describe = s;
		}




	};
#pragma pack(pop,packing)






#endif // !__IDefine

