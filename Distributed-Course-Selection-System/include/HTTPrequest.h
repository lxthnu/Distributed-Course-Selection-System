

#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <regex>

#include "buffer.h"

class HTTPrequest
{
public:
    enum PARSE_STATE{
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH,
    };

    enum HTTP_CODE {
        NO_REQUEST = 0,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURSE,
        FORBIDDENT_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION,
    };

    HTTPrequest() {init();};
    ~HTTPrequest()=default;

    void init();
    bool parse(Buffer& buff); //解析HTTP请求

    //获取HTTP信息
    std::string path() const;
    std::string& path();
    std::string method() const;
    std::string version() const;
    std::string search();
    std::string stu();
    std::string cour();
    std::string getPost(const std::string& key) const;
    std::string getPost(const char* key) const;
    std::string getdata() const;
    std::string getdata_get() const;
    std::string getdata_getV() const;
    int getissearch() const;
    int getsearchres() const;

    bool isKeepAlive() const;

    int post_data_iserror;
    std::string student_id,course_id;

private:
    bool parseRequestLine_(const std::string& line);//解析请求行
    void parseRequestHeader_(const std::string& line); //解析请求头
    void parseDataBody_(const std::string& line); //解析数据体


    void parsePath_();
    int parsePost_();

    static int convertHex(char ch);

    PARSE_STATE state_;
    std::string method_,path_,version_,body_;
    std::unordered_map<std::string,std::string>header_;
    std::unordered_map<std::string,std::string>post_;
    std::unordered_map<std::string,std::string>search_res;
    int success_search,issearch;
    std::string postdata,id,name,KB_get,KB_getv;

    static const std::unordered_set<std::string>DEFAULT_HTML;
   


};





#endif  //HTTP_REQUEST_H