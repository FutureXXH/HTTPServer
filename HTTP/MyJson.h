#ifndef __MYJSON_H
#define __MYJSON_H
#include "IDefine.h"
using namespace std;

class MyJson
{
public:
	inline static void WriteStart(string& str)
	{
		str = "[";
	}
	inline static void WriteEnd(string& str)
	{
		str += "]";
	}
	inline static void WriteValue(string& str, string key, string value)
	{
		if (str.size() < 5)
		{
			str += "{\"" + key + "\":\"" + value + "\"}";
		}
		else
		{

			str += ",{\"" + key + "\":\"" + value + "\"}";
		}
	}
	inline static void WriteValue(string& str, string key,int value)
	{
		if (str.size() < 5)
		{
			str += "{\"" + key + "\":" + to_string(value) + "}";
		}
		else
		{

			str += ",{\"" + key + "\":" + to_string(value) + "}";
		}
	}
	inline static unordered_map<string,string> ParseJson(string str)
	{
		//[{"name":"XXH"},{"value":666}]
		str = deleteString(str, '{');
		str = deleteString(str, '}');
		str = deleteString(str, '[');
		str = deleteString(str, ']');
		str = deleteString(str, '\"');
		vector<string> temp = split(str, ",", true);
		unordered_map<string, string> res;
		for (int i = 0; i < temp.size(); i++)
		{
			vector<string> temp2 = split2(temp[i], ":");
			if (temp2.size() <= 1)
			{
				SERVERPRINT_INFO << "解析json时错误 " << endl;
				continue;
			}
			res.emplace(temp2[0], temp2[1]);

		}
		return res;
	}



};

#endif // !__MYJSON_H
