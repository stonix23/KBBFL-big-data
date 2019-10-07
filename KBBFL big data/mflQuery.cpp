
#include <iostream>
#include <string>

#include "mflQuery.h"


mflQuery::mflQuery(int currentYear)
{
    m_Year = currentYear;
    m_Session = NULL;
    m_Connect = NULL;

}


mflQuery::~mflQuery()
{
    WinHttpCloseHandle(m_Session);
    WinHttpCloseHandle(m_Connect);
}


void mflQuery::mflInit()
{
    // Use WinHttpOpen to obtain a session handle.
    m_Session = WinHttpOpen(L"KBBFL Big Data project 1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);

    if (m_Session == NULL)
    {
        cout << "Couldn't open session.  We're fucked!" << endl;
        return;
    }

    this->mflGetCookie();

    this->mflGetLeagueIDs();

}



void mflQuery::mflGetCookie()
{
    DWORD lastError;
    BOOL  bResults = FALSE;

    wstring username = L"mdstoner23@yahoo.com";
    wstring password = L"password";
    wstring hostname = L"api.myfantasyleague.com";
    wstring leagueYear = to_wstring(m_Year);

    wstring urlParams = L"/" + leagueYear + L"/login?USERNAME=" + username + L"&PASSWORD=" + password + L"&XML=1";

    m_Connect = WinHttpConnect(m_Session, hostname.c_str(), INTERNET_DEFAULT_PORT, 0);

    if (!m_Connect)
    {
        cout << "Couldn't connect to MFL API.  We're fucked!" << endl;
        return;
    }

    HINTERNET hRequest = WinHttpOpenRequest(m_Connect, L"GET", urlParams.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    lastError = GetLastError();

    if (hRequest)
    {
        LPVOID lpBuffer;
        DWORD dwSize = 0, dwIndex = 0;

        bResults = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
        lastError = GetLastError();

        bResults = WinHttpReceiveResponse(hRequest, NULL);
        lastError = GetLastError();

        bResults = WinHttpQueryDataAvailable(hRequest, &dwSize);

        LPSTR pszOutBuffer = new char[dwSize + 1];
        DWORD dwDownloaded = 0;

        // Read the Data.
        memset(pszOutBuffer, 0, dwSize + 1);

        bResults = WinHttpReadData(hRequest, (LPVOID)pszOutBuffer, dwSize, &dwDownloaded);

        // Extract the cookie string
        string httpStr(pszOutBuffer);

        int pos = httpStr.find("MFL_USER_ID=");

        if (pos == string::npos)
        {
            cout << "No cookie found.  We're fucked!" << endl;
            return;
        }

        int cookieStart = httpStr.find_first_of('"', pos);
        int cookieEnd = httpStr.find_first_of('"', cookieStart + 1);
        m_Cookie = httpStr.substr(++cookieStart, cookieEnd - cookieStart);

        cout << "Got cookie - " << m_Cookie << endl;

        delete pszOutBuffer;

        bResults = WinHttpCloseHandle(hRequest);

    }

}


void mflQuery::mflGetLeagueIDs()
{
    DWORD lastError;
    BOOL  bResults = FALSE;

    leagueParams param2013;
    param2013.leagueHostname = L"www53.myfantasyleague.com";
    param2013.leagueID = L"50679";
    m_leagueMap[2013] = param2013;

    leagueParams param2014;
    param2014.leagueHostname = L"www59.myfantasyleague.com";
    param2014.leagueID = L"11851";
    m_leagueMap[2014] = param2014;


    for (int year = 2015; year <= m_Year; year++)
    {
        wstring urlParams = L"/" + to_wstring(year) + L"/export?TYPE=myleagues&JSON=0";

        HINTERNET hRequest = WinHttpOpenRequest(m_Connect, L"GET", urlParams.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
        lastError = GetLastError();

        if (hRequest)
        {
            LPVOID lpBuffer;
            DWORD dwSize = 0, dwIndex = 0;

            bResults = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
            lastError = GetLastError();

            bResults = WinHttpReceiveResponse(hRequest, NULL);
            lastError = GetLastError();

            bResults = WinHttpQueryDataAvailable(hRequest, &dwSize);

            LPSTR pszOutBuffer = new char[dwSize + 1];
            DWORD dwDownloaded = 0;

            // Read the Data.
            memset(pszOutBuffer, 0, dwSize + 1);

            bResults = WinHttpReadData(hRequest, (LPVOID)pszOutBuffer, dwSize, &dwDownloaded);

            wcout << pszOutBuffer << endl;

            //    <league name="KBBFL" url="http://www59.myfantasyleague.com/2019/home/20641" />

            string httpStr(pszOutBuffer);

            int pos = httpStr.find("http:");

            if (pos == string::npos)
            {
                cout << year << " league not found!" << endl;
                continue;
            }

            int hostnameStart = httpStr.find_first_of('w', pos);
            int hostnameEnd = httpStr.find_first_of('/', hostnameStart + 1);
            string queryHostname = httpStr.substr(hostnameStart, hostnameEnd - hostnameStart);

            pos = httpStr.find("home");

            int idStart = httpStr.find_first_of('/', pos);
            int idEnd = httpStr.find_first_of('"', idStart + 1);
            string leagueID = httpStr.substr(++idStart, idEnd - idStart);

            wstring wHostname(queryHostname.begin(), queryHostname.end());
            wstring wLeagueID(leagueID.begin(), leagueID.end());

            leagueParams paramY;
            paramY.leagueHostname = wHostname;
            paramY.leagueID = wLeagueID;
            m_leagueMap[year] = paramY;

            delete pszOutBuffer;
        }

    }

}


void mflQuery::mflRequest(string& outBuffer, int year, wstring requestType, wstring args)
{
    DWORD lastError;
    BOOL  bResults = FALSE;

    HINTERNET hConnect = WinHttpConnect(m_Session, m_leagueMap[year].leagueHostname.c_str(), INTERNET_DEFAULT_PORT, 0);

    if (hConnect)
    {
        wstring urlParams = L"/" + to_wstring(year) + L"/export?TYPE=" + requestType + L"&L=" + m_leagueMap[year].leagueID + args + L"&JSON=0";

        HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", urlParams.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
        lastError = GetLastError();

        if (hRequest)
        {
            LPVOID lpBuffer;
            DWORD dwSize = 0, dwIndex = 0;

            bResults = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
            lastError = GetLastError();

            bResults = WinHttpReceiveResponse(hRequest, NULL);
            lastError = GetLastError();

            bResults = WinHttpQueryDataAvailable(hRequest, &dwSize);

            while (dwSize)
            {
                LPSTR pszOutBuffer = new char[dwSize + 1];
                DWORD dwDownloaded = 0;

                // Read the Data.
                memset(pszOutBuffer, 0, dwSize + 1);

                bResults = WinHttpReadData(hRequest, (LPVOID)pszOutBuffer, dwSize, &dwDownloaded);

                outBuffer.append(pszOutBuffer, dwSize);

                delete pszOutBuffer;

                bResults = WinHttpQueryDataAvailable(hRequest, &dwSize);

            }

            WinHttpCloseHandle(hRequest);

        }

        WinHttpCloseHandle(hConnect);

    }

}



/*


int main()
{

    DWORD lastError;
    BOOL  bResults = FALSE;
    HINTERNET hSession = NULL, hConnect = NULL, hRequest = NULL;

    wstring leagueYear = L"2013";
    wstring username = L"mdstoner23@yahoo.com";
    wstring password = L"fkm1ch";
    wstring hostname = L"api.myfantasyleague.com";

    string queryHostname;
    string leagueID;


    wstring urlParams = L"/" + leagueYear + L"/login?USERNAME=" + username + L"&PASSWORD=" + password + L"&XML=1";


    std::wcout << L"Requesting cookie from: " << hostname << urlParams << endl << endl;

    // Use WinHttpOpen to obtain a session handle.
    hSession = WinHttpOpen(L"KBBFL Big Data project 1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);

    // Specify an HTTP server.
    if (hSession)
    {
        hConnect = WinHttpConnect(hSession, hostname.c_str(), INTERNET_DEFAULT_PORT, 0);

        // Create an HTTP Request handle.
        if (hConnect)
        {
            hRequest = WinHttpOpenRequest(hConnect, L"GET", urlParams.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);

            lastError = GetLastError();

            if (hRequest)
            {
                LPVOID lpBuffer;
                DWORD dwSize = 0, dwIndex = 0;

                bResults = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
                lastError = GetLastError();

                bResults = WinHttpReceiveResponse(hRequest, NULL);
                lastError = GetLastError();


                bResults = WinHttpQueryDataAvailable(hRequest, &dwSize);

                LPSTR pszOutBuffer = new char[dwSize + 1];
                DWORD dwDownloaded = 0;

                // Read the Data.
                ZeroMemory(pszOutBuffer, dwSize + 1);

                bResults = WinHttpReadData(hRequest, (LPVOID)pszOutBuffer, dwSize, &dwDownloaded);

                wcout << pszOutBuffer << endl;

                string httpStr(pszOutBuffer);

                int pos = httpStr.find("MFL_USER_ID=");

                if (pos == string::npos)
                {
                    cout << "No cookie found!" << endl;
                    return 0;
                }

                int cookieStart = httpStr.find_first_of('"', pos);
                int cookieEnd = httpStr.find_first_of('"', cookieStart + 1);
                string cookie = httpStr.substr(++cookieStart, cookieEnd - cookieStart);

                cout << cookie << endl;

            }

            urlParams = L"/" + leagueYear + L"/export?TYPE=myleagues&JSON=0";

            hRequest = WinHttpOpenRequest(hConnect, L"GET", urlParams.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
            lastError = GetLastError();

            if (hRequest)
            {
                LPVOID lpBuffer;
                DWORD dwSize = 0, dwIndex = 0;

                bResults = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
                lastError = GetLastError();

                bResults = WinHttpReceiveResponse(hRequest, NULL);
                lastError = GetLastError();


                bResults = WinHttpQueryDataAvailable(hRequest, &dwSize);

                LPSTR pszOutBuffer = new char[dwSize + 1];
                DWORD dwDownloaded = 0;

                // Read the Data.
                ZeroMemory(pszOutBuffer, dwSize + 1);

                bResults = WinHttpReadData(hRequest, (LPVOID)pszOutBuffer, dwSize, &dwDownloaded);

                wcout << pszOutBuffer << endl;

                //    <league name="KBBFL" url="http://www59.myfantasyleague.com/2019/home/20641" />

                string httpStr(pszOutBuffer);

                if (!wcscmp(leagueYear.c_str(), L"2013"))
                {
                    queryHostname = "www53.myfantasyleague.com";
                    leagueID = "50679";
                }
                else if (!wcscmp(leagueYear.c_str(), L"2014"))
                {
                    queryHostname = "www59.myfantasyleague.com";
                    leagueID = "11851";
                }
                else
                {
                    int pos = httpStr.find("http:");

                    if (pos == string::npos)
                    {
                        cout << "League not found!" << endl;
                        return 0;
                    }

                    int hostnameStart = httpStr.find_first_of('w', pos);
                    int hostnameEnd = httpStr.find_first_of('/', hostnameStart + 1);
                    queryHostname = httpStr.substr(hostnameStart, hostnameEnd - hostnameStart);


                    pos = httpStr.find("home");

                    int idStart = httpStr.find_first_of('/', pos);
                    int idEnd = httpStr.find_first_of('"', idStart + 1);
                    leagueID = httpStr.substr(++idStart, idEnd - idStart);
                }

            }

        }

        wstring wHostname(queryHostname.begin(), queryHostname.end());
        wstring wLeagueID(leagueID.begin(), leagueID.end());

        hConnect = WinHttpConnect(hSession, wHostname.c_str(), INTERNET_DEFAULT_PORT, 0);

        if (hConnect)
        {
            wstring reqType = L"weeklyResults";
            urlParams = L"/" + leagueYear + L"/export?TYPE=" + reqType + L"&L=" + wLeagueID + L"&W=1&JSON=0";

            hRequest = WinHttpOpenRequest(hConnect, L"GET", urlParams.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
            lastError = GetLastError();

            if (hRequest)
            {
                LPVOID lpBuffer;
                DWORD dwSize = 0, dwIndex = 0;

                bResults = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
                lastError = GetLastError();

                bResults = WinHttpReceiveResponse(hRequest, NULL);
                lastError = GetLastError();


                bResults = WinHttpQueryDataAvailable(hRequest, &dwSize);

                while (dwSize)
                {
                    LPSTR pszOutBuffer = new char[dwSize + 1];
                    DWORD dwDownloaded = 0;

                    // Read the Data.
                    ZeroMemory(pszOutBuffer, dwSize + 1);

                    bResults = WinHttpReadData(hRequest, (LPVOID)pszOutBuffer, dwSize, &dwDownloaded);

                    wcout << pszOutBuffer << endl;

                    delete pszOutBuffer;

                    bResults = WinHttpQueryDataAvailable(hRequest, &dwSize);

                }

            }


        }



    }

 
}

*/