#include <iostream>
#include <fstream>
#include <unistd.h>
#include "Curl.hpp"
using namespace std;
string Decode(string Input, int OBFSKEY)
{
    for (unsigned int i = 0; i < Input.size(); i++)
    {
        Input[i] -= 32;
        if (Input[i] < 95)
            Input[i] = (Input[i] + 95 - OBFSKEY) % 95;
        Input[i] += 32;
    }
    return Input;
}
// Copied from OJTool/Luogu.cpp TOOL::LUOGU::GetCSRF
string GetCSRF()
{
    // Get csrf token
    string Token = GetStringBetween(GetDataFromFileToString(),
                                    "<meta name=\"csrf-token\" content=\"", "\"");
    if (Token == "")
        TRIGGER_ERROR("Can not find csrf token");
    return Token;
}

// Copied from OJTool/Luogu.cpp LUOGU::Login
void LoginLuogu(string Username, string Password)
{
    // Check if the user is logged in.
    int HTTPResponseCode = 0;
    cout << "Checking login... " << flush;
    GetDataToFile("https://www.luogu.com.cn/auth/login",
                  "",
                  "",
                  false,
                  "",
                  NULL,
                  &HTTPResponseCode);
    if (HTTPResponseCode == 302)
    {
        cout << "Already logged in" << endl;
        return;
    }
    cout << "Not logged in" << endl;

    string Token = GetCSRF();
    int ErrorCounter = 0;
    while (1)
    {
        // Get login captcha
        cout << "Getting login captcha... " << flush;
        GetDataToFile("https://www.luogu.com.cn/api/verify/captcha",
                      "",
                      "/tmp/Captcha.jpeg");
        cout << "Succeed" << endl;

        // Predict captcha
        curl_slist *HeaderList = NULL;
        HeaderList = curl_slist_append(HeaderList, "Content-Type: application/json");
        cout << "Predicting captcha by web api... " << flush;
        string Captcha = "";
        try
        {
            GetDataToFile("https://luogu-captcha-bypass.piterator.com/predict/",
                          "",
                          "",
                          true,
                          "data:image/jpeg;base64," +
                              Base64Encode(
                                  GetDataFromFileToString("/tmp/Captcha.jpeg")),
                          HeaderList);
            cout << "Succeed" << endl;
            Captcha = GetDataFromFileToString();
        }
        catch (CLNException &Exception)
        {
            cout << "Failed" << endl
                 << "Predicting captcha by AI model... " << flush;
            if (system("python OJTool/PredictLuoguCaptcha.py > /dev/null 2>&1") == 0)
            {
                cout << "Succeed" << endl;
                Captcha = FixString(GetDataFromFileToString("/tmp/Captcha.txt"));
            }
            else
            {
                cout << "Failed" << endl;
                system("code-insiders /tmp/Captcha.jpeg");
                cout << "Please input the captcha: ";
                cin >> Captcha;
            }
        }

        // Create a json object to store the login request info
        json LoginRequest;
        LoginRequest["username"] = Username;
        LoginRequest["password"] = Password;
        LoginRequest["captcha"] = Captcha;

        // Create a header list for the curl request
        HeaderList = NULL;
        HeaderList = curl_slist_append(HeaderList, string("X-CSRF-TOKEN: " + Token).c_str());
        HeaderList = curl_slist_append(HeaderList, string("Content-Length: " +
                                                          to_string(LoginRequest.dump().size()))
                                                       .c_str());
        HeaderList = curl_slist_append(HeaderList, "Host: www.luogu.com.cn");
        HeaderList = curl_slist_append(HeaderList, "Referer: https://www.luogu.com.cn/auth/login");
        HeaderList = curl_slist_append(HeaderList, "Origin: https://www.luogu.com.cn");
        HeaderList = curl_slist_append(HeaderList, "X-Requested-With: XMLHttpRequest");

        // Send the login request to the server
        cout << "Logging in... " << flush;
        GetDataToFile("https://www.luogu.com.cn/api/auth/userPassLogin",
                      "",
                      "",
                      true,
                      LoginRequest.dump(),
                      HeaderList);

        // Parse the response to a json object
        json LoginInfo = json::parse(GetDataFromFileToString());
        if (!LoginInfo["status"].is_null())
        {
            // If the captcha is incorrect and the error counter is less than 5, try again.
            if (LoginInfo["data"].as_string() != "验证码错误" && ErrorCounter < 5)
            {
                TRIGGER_ERROR_WITH_CODE_AND_MESSAGE("Login failed",
                                                    LoginInfo["status"].as_integer(),
                                                    LoginInfo["data"].as_string());
            }
            else
                cout << "Failed (Captcha check failed for " << ErrorCounter + 1 << " times)" << endl;
        }
        else
        {
            cout << "Succeed" << endl;
            break;
        }
        ErrorCounter++;
    }
}
void Login(string Username, string Password)
{
    LoginLuogu(Username, Password);
    cout << "Logging luogu class... " << flush;
    int HTTPResponseCode = 0;
    GetDataToFile("https://class.luogu.com.cn/course", "Header.tmp", "Body.tmp", false, "", NULL, &HTTPResponseCode);
    if (HTTPResponseCode != 200)
    {
        GetDataToFile("https://www.luogu.com.cn/api/OAuth2/authorize?response_type=code&client_id=lgclass.luoguclass&scope=oauth2.login&redirect_uri=https%3A%2F%2Fclass.luogu.com.cn%2Flogin%2Fcheck-luogu");
        string RedirectURL = FindLocation();
        GetDataToFile(RedirectURL);
    }
    cout << "Success" << endl;
}
void DownloadVideo(string CourseID)
{
    GetDataToFile(string("https://class.luogu.com.cn/classroom/" + CourseID + "?_contentOnly=1"));
    json CourseInfo = json::parse(GetDataFromFileToString());
    if (CourseInfo["code"].as_integer() != 200)
    {
        TRIGGER_ERROR_WITH_CODE_AND_MESSAGE("Get course info", CourseInfo["code"].as_integer(), CourseInfo["currentData"]["errorMessage"].as_string());
        return;
    }
    unsigned int M3U8Counter = 0;
    for (json::iterator jit = CourseInfo["currentData"]["replayFiles"].begin(); jit != CourseInfo["currentData"]["replayFiles"].end(); jit++)
    {
        GetDataToFile(
            Decode(jit.value()["url"]["HD"].as_string(),
                   CourseInfo["currentData"]["obfsKey"].as_integer()),
            "Header.tmp",
            "Body.tmp",
            false,
            "",
            NULL,
            NULL,
            "application/json",
            "",
            true);
        string M3U8Detail = GetDataFromFileToString();
        unsigned int TSCounter = 0;
        if (system(string("mkdir \"" + CourseInfo["currentData"]["lesson"]["name"].as_string() + "\"").c_str()) != 0)
        {
            TRIGGER_ERROR("Create dir failed");
        }
        ofstream OutputFileStream(string(CourseInfo["currentData"]["lesson"]["name"].as_string() + "/index.m3u8"));
        if (OutputFileStream.bad())
        {
            TRIGGER_ERROR("Open output file failed");
        }
        queue<string> TsUrlList;
        for (unsigned int i = 0; i < M3U8Detail.size(); i++)
        {
            unsigned int LineStartPos = i;
            unsigned int LineEndPos = i + 1;
            while (LineEndPos < M3U8Detail.size() && M3U8Detail[LineEndPos] != '\n')
                LineEndPos++;
            string Line = M3U8Detail.substr(LineStartPos, LineEndPos - LineStartPos);
            if (Line.size() > 0)
            {
                if (Line[0] != '#')
                    TSCounter++;
                else if (Line.find("https://class.luogu.com.cn/") != string::npos && Line.find("https://class.luogu.com.cn/api/") == string::npos)
                    Line.replace(Line.find("https://class.luogu.com.cn/"), 27, "https://class.luogu.com.cn/api/");
            }
            TsUrlList.push(Line);
            i = LineEndPos;
        }
        int CurrentTSIndex = 0;
        while (!TsUrlList.empty())
        {
            if (TsUrlList.front()[0] == '#')
                OutputFileStream << TsUrlList.front() << endl;
            else
            {
                GetDataToFile(string("https://class.luogu.com.cn/api/live/signReplay?url=https://video.class.luogu.com.cn/yugu-live/" + CourseID + "/" + TsUrlList.front()));
                json TsUrlInfo = json::parse(GetDataFromFileToString());
                cout << "\r" << CurrentTSIndex << "/" << TSCounter;
                fflush(stdout);
                try
                {
                    GetDataToFile(TsUrlInfo["url"].as_string(), "Header.tmp", string(CourseInfo["currentData"]["lesson"]["name"].as_string() + "/" + to_string(CurrentTSIndex) + ".ts"));
                }
                catch (CLNException)
                {
                    CurrentTSIndex--;
                    continue;
                }
                OutputFileStream << CurrentTSIndex << ".ts" << endl;
                CurrentTSIndex++;
            }
            TsUrlList.pop();
        }
        OutputFileStream.close();
        if (system((string("ffmpeg -y -hide_banner -loglevel error -protocol_whitelist concat,file,http,https,tcp,tls,crypto -i ") +
                    "\"" + CourseInfo["currentData"]["lesson"]["name"].as_string() + "/index.m3u8\" " +
                    "\"" + CourseInfo["currentData"]["lesson"]["name"].as_string() + "/index.mp4\"")
                       .c_str()) == 0 &&
            system((string("cp ") +
                    "\"" + CourseInfo["currentData"]["lesson"]["name"].as_string() + "/index.mp4\" " +
                    "\"" + CourseInfo["currentData"]["lesson"]["name"].as_string() +
                    (CourseInfo["currentData"]["replayFiles"].size() == 1
                         ? ""
                         : "_" + to_string(M3U8Counter)) +
                    ".mp4\"")
                       .c_str()) == 0)
            system(string("rm -r \"" + CourseInfo["currentData"]["lesson"]["name"].as_string() + "\"").c_str());
        M3U8Counter++;
    }
}
int main(int argc, char **argv)
{
    CLN_TRY
    string Username = "";
    string Password = "";
    string CourseID = "";
    for (int i = 1; i < argc; i++)
    {
        string Argument = argv[i];
        string NextArgument = i + 1 == argc ? "" : argv[i + 1];
        if (Argument == "-u" || Argument == "--username")
        {
            Username = NextArgument;
            i++;
        }
        else if (Argument == "-p" || Argument == "--password")
        {
            Password = NextArgument;
            i++;
        }
        else if (Argument == "-c" || Argument == "--course")
        {
            CourseID = NextArgument;
            i++;
        }
        else
            TRIGGER_ERROR("Unknown option \"" + Argument + "\"");
    }
    if (Username == "")
        TRIGGER_ERROR("No username provided");
    if (Password == "")
        TRIGGER_ERROR("No password provided");
    if (CourseID == "")
        TRIGGER_ERROR("No course provided");

    Login(Username, Password);
    DownloadVideo(CourseID);
    CLN_CATCH return 0;
}
