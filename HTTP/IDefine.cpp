#include "IDefine.h"

char FileExePath[MAX_EXE_LEN];
void initPath()
{
	memset(FileExePath, 0, MAX_EXE_LEN);
#ifdef ___WIN32_
	GetModuleFileNameA(NULL, (LPSTR)FileExePath, MAX_EXE_LEN);
	std::string str(FileExePath);
	size_t pos = str.find_last_of("\\");
	str = str.substr(0, pos + 1);
	
#else
	int  res = readlink("/proc/self/exe", FileExePath, MAX_EXE_LEN);
	std::string str(FileExePath);
	size_t pos = str.find_last_of("/");
	str = str.substr(0, pos + 1);

#endif // ___WIN32_
	memcpy(FileExePath, str.c_str(), MAX_EXE_LEN);
	SERVERPRINT_INFO << "file:" << FileExePath << std::endl;
}

std::string deleteString(std::string s, char c)
{
	std::string str;
	if (!s.empty())
	{
		int index = 0;
		while ((index = s.find(c,index)) >= 0)
		{
			s.erase(index, 1);
		}

		str = s;
	}

	return str;
}

std::vector<std::string> split(std::string str, std::string pattern, bool isadd)
{
	int pos = 0;
	std::vector<std::string> res;
	//是否包含尾部
	if (isadd)
	{
		str += pattern;
	}
	int size = str.size();
	for (int i = 0; i < size; i++)
	{
		pos = str.find(pattern,i);
		if (pos < size)
		{
			res.emplace_back(str.substr(i, pos - i));
			i = pos + pattern.size() - 1;
		}

	}



	return res;
}
//只分成两部分
std::vector<std::string> split2(std::string str, std::string pattern)
{
	int pos = 0;
	std::vector<std::string> res;
	int size = str.size();

	pos = str.find(pattern);
	if (pos < 0)return res;
	if (pos + 1 >= size)return res;
	res.emplace_back(str.substr(0, pos));
	res.emplace_back(str.substr(pos+1));

	return res;
}

bool is_file(const std::string& path)
{
   struct stat st;

	return stat(path.c_str(),&st) >= 0 && S_ISREG(st.st_mode);
}

void read_file(const std::string& path, std::string& out)
{
	std::ifstream fs(path, std::ios_base::binary);
	fs.seekg(0, std::ios_base::end);

	auto size = fs.tellg();
	fs.seekg(0);
	out.resize((size_t)size);
	fs.read(&out[0], (std::streamsize)size);


}

bool read_Quest(const std::string& fileName, std::string& out)
{
	std::string filepath(FileExePath);
#ifdef ___WIN32_
	std::string sub_path = filepath + "res\\" + fileName;
#else
	std::string sub_path = filepath + "res/" + fileName;
#endif // ___WIN32_



	if (sub_path.back() == '/')
	{
		sub_path += "index.html";
	}
	if (is_file(sub_path))
	{
		read_file(sub_path, out);
		return true;
	}
	return false;
}

bool write_File(const std::string& filename, char* c, int len)
{


#ifdef ___WIN32_
	std::string path(FileExePath);
	path += "res\\" + filename;
#else
	std::string path(FileExePath);
	path += "res/" + filename;
#endif // ___WIN32_

	std::ofstream fs(path, std::ios_base::binary);
	if (!fs)return -1;
	fs.write(c, len);
	return 0;


}
