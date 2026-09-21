#include <Rpc.h>
#include "IniReader.h"

#define MAX_STR_LEN 1024

CIniReader::CIniReader(const std::string& path)
{
    m_strPath = std::string(path);
}

CIniReader::~CIniReader()
{

}

float CIniReader::ReadFloat(const std::string& strSection, const std::string& strKey, float fDefault)
{
    char szDefaultBuf[MAX_STR_LEN];
    ZeroMemory(szDefaultBuf, sizeof(szDefaultBuf));
    sprintf(szDefaultBuf, "%f", fDefault);

    float fResult = atof(szDefaultBuf);

    std::string str = ReadStringA(strSection, strKey, szDefaultBuf);
    if (strcmp(str.c_str(), szDefaultBuf) != 0)
        fResult = atof(str.c_str());

    //printf("%s => [%f]\n", strKey.c_str(), fResult);
    return fResult;
}

int CIniReader::ReadInt(const std::string& strSection, const std::string& strKey, int nDefault)
{
    int result = GetPrivateProfileIntA(strSection.c_str(), strKey.c_str(), nDefault, m_strPath.c_str());
    //printf("%s => [%d]\n", strKey.c_str(), result);
    return result;
}

bool CIniReader::ReadBool(const std::string& strSection, const std::string& strKey, bool bDefault)
{
    char szDefaultBuf[MAX_STR_LEN];
    ZeroMemory(szDefaultBuf, sizeof(szDefaultBuf));
    sprintf(szDefaultBuf, "%s", (bDefault) ? "true" : "false");

    const std::string& str = ReadStringA(strSection, strKey, szDefaultBuf, false);

    char szStrBuf[MAX_STR_LEN];
    ZeroMemory(szStrBuf, sizeof(szStrBuf));

    for (int i = 0; i < str.length(); i++)
        szStrBuf[i] = (char)tolower(str.c_str()[i]);

    bool result = (strcmp(szStrBuf, "true") == 0);
    //printf("%s => [%s]\n", strKey.c_str(), result ? "true" : "false");

    return result;
}

std::string CIniReader::ReadStringA(const std::string& strSection, const std::string& strKey, const std::string& strDefault, bool printValue)
{
    char szBuffer[MAX_STR_LEN] = { 0 };
    GetPrivateProfileStringA(strSection.c_str(), strKey.c_str(), strDefault.c_str(), szBuffer, MAX_STR_LEN, m_strPath.c_str());

    std::string result = std::string(szBuffer);
    //if (printValue)
    //	printf("%s => [%s]\n", strKey.c_str(), result.c_str());
    return result;
}
