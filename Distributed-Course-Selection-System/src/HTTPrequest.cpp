

#include "../include/HTTPrequest.h"

int p1=0;

const std::unordered_set<std::string> HTTPrequest::DEFAULT_HTML{"/index"};

void HTTPrequest::init() {
    method_ = path_ = version_ = body_ = "";
    state_ = REQUEST_LINE;
    header_.clear();
    post_.clear();
    search_res.clear();
    success_search=0;
    post_data_iserror=0;
    id="";
    name="";
    issearch=0;
    postdata="";
}

bool HTTPrequest::isKeepAlive() const {
    if(header_.count("Connection") == 1) {
        return header_.find("Connection")->second == "keep-alive" && version_ == "1.1";
    }
    return false;
}

bool HTTPrequest::parse(Buffer& buff) {
    const char CRLF[] = "\r\n";
    if(buff.readableBytes() <= 0) {
        return false;
    }

    while(buff.readableBytes() && state_ != FINISH) {
        const char* lineEnd = std::search(buff.curReadPtr(), buff.curWritePtrConst(), CRLF, CRLF + 2);      
        std::string line(buff.curReadPtr(), lineEnd);

        switch(state_)
        {
        case REQUEST_LINE:
            // std::cout<<"REQUEST: "<<line<<std::endl;
            //POST /api/echo HTTP/1.1
            if(!parseRequestLine_(line)) {//获得method_,path_,version_，并将state_置为HEADERS
                return false;
            }
            parsePath_();
            break;    
        case HEADERS:
            // if(p1)std::cout<<"readHEADERS:"<<line<<std::endl;
            /*Host: localhost:8080 User-Agent: curl/7.68.0 Accept: \*\/\*  
            Content-Type: application/x-www-form-urlencoded  Content-Length: 13
            */
            //读请求头部剩下的部分
            parseRequestHeader_(line);
            if(buff.readableBytes() <= 2) {
                state_ = FINISH;
            }
            break;
        case BODY:
            // if(p1)std::cout<<"readBODY:"<<line<<std::endl;
            parseDataBody_(line);
            break;
        default:
            break;
        }
        if(lineEnd == buff.curWritePtr()) { break; }
        buff.updateReadPtrUntilEnd(lineEnd + 2);
    }
    //header_
    
    return true;
}

void HTTPrequest::parsePath_() {
    if(path_ == "/") {
        path_ = "/index.html"; 
    }
    else {
        for(auto &item: DEFAULT_HTML) {
            if(item == path_) {
                path_ += ".html";
                break;
            }
        }
    }
}

bool HTTPrequest::parseRequestLine_(const std::string& line) {
    std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    std::smatch subMatch;
    if(regex_match(line, subMatch, patten)) {   
        method_ = subMatch[1];
        path_ = subMatch[2];
        version_ = subMatch[3];
        state_ = HEADERS;
        return true;
    }
    return false;
}
void HTTPrequest::parseRequestHeader_(const std::string& line) {
    std::regex patten("^([^:]*): ?(.*)$");
    std::smatch subMatch;
    if(regex_match(line, subMatch, patten)) {//如果匹配到：，？
        // std::cout<<"==="<<subMatch[1]<<" "<<subMatch[2]<<std::endl;
        header_[subMatch[1]] = subMatch[2];
    }
    else {//没有则进入数据部分
        state_ = BODY;
    }
}

void HTTPrequest::parseDataBody_(const std::string& line) {
    body_ = line;
    parsePost_();
    state_ = FINISH;
}

int HTTPrequest::convertHex(char ch) {
    if(ch >= 'A' && ch <= 'F') return ch -'A' + 10;
    if(ch >= 'a' && ch <= 'f') return ch -'a' + 10;
    return ch;
}

//post处理
int HTTPrequest::parsePost_() {
    if(method_ == "POST" ) {
        
        //没有数据直接返回
        if(body_.size() == 0) { return 0; }

        std::string key="", value="",keyname="",valuename="";//id:name
        int num = 0,num1=0;
        int n = body_.size();
        int i = 0, j = 0;
        int iserro=0;
        if(header_["Content-Type"] == "application/x-www-form-urlencoded")
		{
        std::regex patten("^id=.+&name=.+$");
		std::smatch subMatch;
		if(regex_match(body_, subMatch, patten)) {//如果匹配到：，？
        // for (size_t i = 1; i < subMatch.size(); ++i)
        //     {
        //         std::cout <<"==="<< subMatch[i] << " ";
        //     }
        post_data_iserror=0;
    	}
		else{
		post_data_iserror=1;
		}
    	}

		if(header_["Content-Type"] == "application/json")
		{
		std::regex r("^\\{\\\"student_id\\\":(.*),\\\"course_id\\\":(.*)\\}$");
        std::smatch matchResult;
		if(regex_match(body_,matchResult,  r)) {

        // for (size_t i = 1; i < matchResult.size(); ++i)
        //     {
        //         std::cout <<"==="<< matchResult[i] << " ";
        //     }
        std::string strkv="SET ";
        student_id=matchResult[1];
        course_id=matchResult[2];

        KB_get="GET ";
        KB_get+=student_id;
        KB_getv=course_id;
        
        if(path_=="/api/drop") strkv="DEL ";

        // std::cout<<k<<" "<<v<<std::endl;
        strkv+= student_id;
        strkv+=" ";
        strkv+=course_id;
        // std::cout<<strkv<<std::endl;
        // body_="SET KEY VAL";

        post_data_iserror=0;
        body_=strkv;
    	}
		else{

		post_data_iserror=1;
		}
    	}
        // printf("pd:%d\n",post_data_iserror);
      
    }
	return 1;   
}

std::string HTTPrequest::path() const{
    return path_;
}

std::string& HTTPrequest::path(){
    return path_;
}
std::string HTTPrequest::method() const {
    return method_;
}
std::string HTTPrequest::getdata() const {
    return body_;
}

std::string HTTPrequest::getdata_get() const {
    return KB_get;
}

std::string HTTPrequest::getdata_getV() const {
    return KB_getv;
}

std::string HTTPrequest::version() const {
    return version_;
}

std::string HTTPrequest::stu() {
    return student_id;
}

std::string HTTPrequest::cour() {
    return course_id;
}

int HTTPrequest::getissearch() const{
    return issearch;
}

int HTTPrequest::getsearchres() const{
    return success_search;
}

std::string HTTPrequest::getPost(const std::string& key) const {
    assert(key != "");
    if(post_.count(key) == 1) {
        return post_.find(key)->second;
    }
    return "";
}
std::string HTTPrequest::search() {
    std::unordered_map<std::string, std::string>::iterator i=search_res.begin();
    std::string res="[";
    char tmpstr[64];
    
    for(i=search_res.begin();i!=search_res.end();i++){
        std::string t1=i->first,t2=i->second;
        snprintf(tmpstr,64,"{\"id\":%s,\"name\":%s}",t1.c_str(),t2.c_str());
        res+=tmpstr;

    }
    res+="]";
    if(p1)std::cout<<"res: "<<res<<std::endl;
    return res;
}
