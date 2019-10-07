#pragma once
#include <Windows.h>
#include <windef.h>
#include <winhttp.h>

#include <string>
#include <unordered_map>

#include "pugixml.hpp"

using namespace std;

struct leagueParams
{
    wstring leagueID;
    wstring leagueHostname;
};


class mflQuery
{
private:

    DWORD m_Year;
    HINTERNET m_Session;
    HINTERNET m_Connect;
    string m_Cookie;

    unordered_map<int, leagueParams> m_leagueMap;

public:

    mflQuery(int currentYear);

    ~mflQuery();

    void mflInit();

    void mflGetCookie();

    void mflGetLeagueIDs();

    void mflRequest(string& outBuffer, int year, wstring requestType, wstring args);

};
