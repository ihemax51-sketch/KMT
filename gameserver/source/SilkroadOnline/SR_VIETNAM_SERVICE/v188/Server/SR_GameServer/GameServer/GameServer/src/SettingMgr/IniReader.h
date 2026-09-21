#pragma once


#include <string>

class CIniReader
{
private:
    std::string m_strPath;

public:
    CIniReader(const std::string& path);
    ~CIniReader();

    float ReadFloat(const std::string& strSection, const std::string& strKey, float fDefault);
    int ReadInt(const std::string& strSection, const std::string& strKey, int nDefault);
    bool ReadBool(const std::string& strSection, const std::string& strKey, bool bDefault);
    std::string ReadStringA(const std::string& strSection, const std::string& strKey, const std::string& strDefault, bool printValue = true);
};