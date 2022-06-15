#include "../include/HTTPresponse.h"

const std::unordered_map<std::string, std::string> HTTPresponse::SUFFIX_TYPE = {
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".word",  "application/nsword" },
    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".au",    "audio/basic" },
    { ".mpeg",  "video/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".css",   "text/css "},
    { ".js",    "text/javascript "},
};

const std::unordered_map<int, std::string> HTTPresponse::CODE_STATUS = {
    { 200, "OK" },
    { 400, "Bad Request" },
    { 403, "Forbidden" },
    { 404, "Not Found" },
    { 501, "Not Implemented" }
};

const std::unordered_map<int, std::string> HTTPresponse::CODE_PATH = {
    { 400, "/400.html" },
    { 403, "/403.html" },
    { 404, "/404.html" },
    { 501, "/501.html" }
};

HTTPresponse::HTTPresponse() {
    code_ = -1;
    path_ =  "";
    srcDir_ ="./static";
    isKeepAlive_ = false;
    mmFile_ = nullptr; 
    mmFileStat_ = { 0 };
};

HTTPresponse::~HTTPresponse() {
    unmapFile_();
}

void HTTPresponse::init(const std::string& srcDir, std::string& path, bool isKeepAlive, std::string s,std::string d,int code,int t){
    // assert(srcDir != "");
    if(mmFile_) { unmapFile_(); }
    code_ = code;
    isKeepAlive_ = isKeepAlive;
    path_ = path;
    srcDir_ = srcDir;
    type=t;
    ptype=s;
    data=d;
    mmFile_ = nullptr; 
    mmFileStat_ = { 0 };
}

void HTTPresponse::makeResponse(Buffer& buff) {
    /* 判断请求的资源文件 */
    if(type==1){//get  
    if(stat((srcDir_ + path_).data(), &mmFileStat_) < 0 || S_ISDIR(mmFileStat_.st_mode)) {
        code_ = 404;
    }
    else if(!(mmFileStat_.st_mode & S_IROTH)) {
        code_ = 403;
    }
    }

    if(code_ == -1) { 
        code_ = 200; 
    }
    
    errorHTML_();
    addStateLine_(buff);
    addResponseHeader_(buff);
    addResponseContent_(buff);
}

char* HTTPresponse::file() {
    return mmFile_;
}

size_t HTTPresponse::fileLen() const {
    return mmFileStat_.st_size;
}

void HTTPresponse::errorHTML_() {
    if(CODE_PATH.count(code_) == 1) {
        path_ = CODE_PATH.find(code_)->second;
        stat((srcDir_ + path_).data(), &mmFileStat_);
    }
}

void HTTPresponse::addStateLine_(Buffer& buff) {
    std::string status;
    if(CODE_STATUS.count(code_) == 1) {
        status = CODE_STATUS.find(code_)->second;
    }
    else {
        code_ = 400;
        status = CODE_STATUS.find(400)->second;
    }
    buff.append("HTTP/1.1 " + std::to_string(code_) + " " + status + "\r\n");
}

void HTTPresponse::addResponseHeader_(Buffer& buff) {
    // buff.append("Connection: ");
    // if(isKeepAlive_) {
    //     buff.append("keep-alive\r\n");
    //     buff.append("keep-alive: max=6, timeout=120\r\n");
    // } else{
    //     buff.append("close\r\n");
    // }
    if(ptype=="")
        buff.append("Content-type: " + getFileType_() + "\r\n");
    else buff.append("Content-type: " + ptype + "\r\n");
}

void HTTPresponse::addResponseContent_(Buffer& buff) {
    if(srcDir_==""||path_==""||data!=""){
        buff.append("Content-length: " + std::to_string(data.length()) + "\r\n\r\n");
        buff.append(data);
		// return;
    }
    else{
    int srcFd = open((srcDir_ + path_).data(), O_RDONLY);
    if(srcFd < 0) { 
        errorContent(buff, "File NotFound!");
        return; 
    }

    // 将文件映射到内存提高文件的访问速度 
    // MAP_PRIVATE 建立一个写入时拷贝的私有映射
    int* mmRet = (int*)mmap(0, mmFileStat_.st_size, PROT_READ, MAP_PRIVATE, srcFd, 0);
    if(*mmRet == -1) {
        errorContent(buff, "File NotFound!");
        return; 
    }
    mmFile_ = (char*)mmRet;
    close(srcFd);
    buff.append("Content-length: " + std::to_string(mmFileStat_.st_size) + "\r\n\r\n");
    }
}

void HTTPresponse::unmapFile_() {
    if(mmFile_) {
        munmap(mmFile_, mmFileStat_.st_size);
        mmFile_ = nullptr;
    }
}

std::string HTTPresponse::getFileType_() {
    /* 判断文件类型 */
    std::string::size_type idx = path_.find_last_of('.');
    if(idx == std::string::npos) {
        return "text/plain";
    }
    std::string suffix = path_.substr(idx);
    if(SUFFIX_TYPE.count(suffix) == 1) {
        return SUFFIX_TYPE.find(suffix)->second;
    }
    return "text/plain";
}

void HTTPresponse::errorContent(Buffer& buff, std::string message) 
{
    std::string body;
    std::string status;
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    if(CODE_STATUS.count(code_) == 1) {
        status = CODE_STATUS.find(code_)->second;
    } else {
        status = "Bad Request";
    }
    body += std::to_string(code_) + " : " + status  + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>TinyWebServer</em></body></html>";

    buff.append("Content-length: " + std::to_string(body.size()) + "\r\n\r\n");
    buff.append(body);
}
