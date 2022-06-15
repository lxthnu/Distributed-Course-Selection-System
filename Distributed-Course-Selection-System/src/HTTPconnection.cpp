#include "../include/HTTPconnection.h"
#include <regex>
#include "../include/core.hpp"
#include "../include/coordinate.h"
#include "../include/participant.h"
#include "../include/TCPServer.h"
#include "../include/conf.hpp"
#include "../include/parser.hpp"
#include "../include/ossSocket.hpp"
#include "../include/util.hpp"
#include "../include/global.h"

const char* HTTPconnection::srcDir;
std::atomic<int> HTTPconnection::userCount;
bool HTTPconnection::isET;
#include "../include/json.hpp"
using json = nlohmann::json;
extern json json_data,json_studata,json_codata;

HTTPconnection::HTTPconnection() { 
    fd_ = -1;
    addr_ = { 0 };
    isClose_ = false;
    has_open_tcpserver=0;
    has_get_map=0;
    has_get_tcp=0;
    has_get_cluster=0;
    s1_port=8001;
    s2_port=8006;
    s1_addr="127.0.0.1";
    s2_addr="127.0.0.1";
    srcDir="./static";
};

HTTPconnection::~HTTPconnection() { 
    closeHTTPConn(); 
};

void HTTPconnection::initHTTPConn(int fd, const sockaddr_in& addr) {
    assert(fd > 0);
    userCount++;
    addr_ = addr;
    fd_ = fd;
    writeBuffer_.initPtr();
    readBuffer_.initPtr();
    isClose_ = false;
}

void HTTPconnection::closeHTTPConn() {
    response_.unmapFile_();
    if(isClose_ == false){
        isClose_ = true; 
        userCount--;
        close(fd_);
    }
}

int HTTPconnection::getFd() const {
    return fd_;
};

struct sockaddr_in HTTPconnection::getAddr() const {
    return addr_;
}

const char* HTTPconnection::getIP() const {
    return inet_ntoa(addr_.sin_addr);
}

int HTTPconnection::getPort() const {
    return addr_.sin_port;
}

ssize_t HTTPconnection::readBuffer(int* saveErrno) {
    ssize_t len = -1;
    do {
        len = readBuffer_.readFd(fd_, saveErrno);
        //std::cout<<fd_<<" read bytes:"<<len<<std::endl;
        if (len <= 0) {
            break;
        }
    } while (isET);
    return len;
}

ssize_t HTTPconnection::writeBuffer(int* saveErrno) {
    ssize_t len = -1;
    do {
        len = writev(fd_, iov_, iovCnt_);
        if(len <= 0) {
            *saveErrno = errno;
            break;
        }
        if(iov_[0].iov_len + iov_[1].iov_len  == 0) { break; } /* 传输结束 */
        else if(static_cast<size_t>(len) > iov_[0].iov_len) {
            iov_[1].iov_base = (uint8_t*) iov_[1].iov_base + (len - iov_[0].iov_len);
            iov_[1].iov_len -= (len - iov_[0].iov_len);
            if(iov_[0].iov_len) {
                writeBuffer_.initPtr();
                iov_[0].iov_len = 0;
            }
        }
        else {
            iov_[0].iov_base = (uint8_t*)iov_[0].iov_base + len; 
            iov_[0].iov_len -= len; 
            writeBuffer_.updateReadPtr(len);
        }
    } while(isET || writeBytes() > 10240);
    return len;
}

int hasdbmsg=0;
string msg="";

void getMsg(TCPServer tcp){
    while(1){
        tcp.accepted();
    }
}

void outMsg(TCPServer tcp){
    while(1){
        queue<descript_socket*> descT=tcp.getMessage();//从服务器的信息队列中取出信息，一次性最多只能取10条
        if(!descT.empty()){
            descript_socket* desc=descT.front();
            msg=desc->message;
            hasdbmsg=1;            
        }      
    }
}

void HTTPconnection::open_tcpserver(){
    std::thread p1(getMsg,std::move(tcp));
    std::thread p2(outMsg,std::move(tcp));
    p1.detach();//detach()后子线程在后台独立继续运行
    p2.detach();

}


bool HTTPconnection::handleHTTPConn() {
    // printf("===1\n");
    // if(has_open_tcpserver==0){
    //     open_tcpserver();
    //     has_open_tcpserver=1;
    // }
    std::thread p1(getMsg,std::move(tcp));
    std::thread p2(outMsg,std::move(tcp));
    p1.detach();//detach()后子线程在后台独立继续运行
    p2.detach();

    request_.init();
    if(readBuffer_.readableBytes() <= 0) {
        //std::cout<<"readBuffer is empty!"<<std::endl;
        return false;
    }
    else if(request_.parse(readBuffer_)) {
        // cout<<"path:"<<request_.path()<<endl;
        std::string dir="";
        string kb_getv1="",kb_getv2="";
        std::smatch matchResult;
        TCPClient tcpc;
        if(request_.method()=="POST"){
            
            // printf("post\n");
            if(request_.path()=="/api/echo"){
                if(request_.post_data_iserror==0){
                    response_.init(dir, request_.path(), request_.isKeepAlive(),"application/x-www-form-urlencoded",request_.getdata(),200,2);
                }
                else{
					std::string dpath="/error.txt";
                    response_.init(srcDir,dpath, request_.isKeepAlive(),"text/plain","",404,2);
                }

            }
            else if(request_.path()=="/api/choose"){
                if(request_.post_data_iserror==0){
					// std::cout << "dd:"<<request_.getdata()<<std::endl;
                    std::string s="application/json";

                    std::string tmsg=request_.getdata();
                    std::string tmsg2=request_.getdata_get();
                    std::regex r(".*ok.*"),r2(".*error_find.*");

                    std::vector<std::string> list,list2;
                    split(tmsg, list, ' ');//SET A B
                    Parser p;
                    std::string pac = p.getRESPArry(list);//把原始字符串转换成RESP形式

                    split(tmsg2, list2, ' ');//SET A B
                    
                    std::string pac2 = p.getRESPArry(list2);//把原始字符串转换成RESP形式

                    //选课前先查看选课人数达到上限没
                    
                    
                    // //建立客户端，创建套接字并绑定到KV服务器，然后发送命令
                    string getsel="GET kb2";
                    getsel+=request_.course_id;
                    std::vector<std::string> getsel_list;
                    split(getsel, getsel_list, ' ');
                    std::string getsel_pac = p.getRESPArry(getsel_list);
                    
                    

                                       
                    if(cmap.find(request_.course_id)==cmap.end()){//没有该课程ID
                        // string errmsg="";
                        string respos="{\"status\":\"error\",\"message\":\"没有该课程ID\"}";
                        // respos+=errmsg;
                        std::string dpath="/error.json";
                        response_.init(dir, dpath, request_.isKeepAlive(),"application/json",respos,403,2);

                    }                   
                    else if(smap.find(request_.student_id)==smap.end()){//没有该学生ID
                        // string errmsg="";
                        string respos="{\"status\":\"error\",\"message\":\"没有该学生ID\"}";
                        // respos+=errmsg;
                        std::string dpath="/error.json";
                        response_.init(dir, dpath, request_.isKeepAlive(),"application/json",respos,403,2);

                    }
                    else{
                    cout<<"与集群："<<s2_addr<<":"<<s2_port<<"建立连接"<<endl;
                    tcpc.setup(s2_addr,s2_port);                   
                    tcpc.Send(getsel_pac);                  
                    tcpc.exit();
                    char c_getsel[10];
                    int c_idx=0;
                    while (1)
                    {
                        if(hasdbmsg==1){
                        int len=msg.length();
                        int idx=0;
                        for(;idx<len;idx++){
                            if(msg[idx]=='/')break;
                        }
                        for(idx=idx+1;idx<len;idx++){
                            if(msg[idx]=='/')break;
                            else {
                                c_getsel[c_idx]=msg[idx];
                                c_idx+=1;
                            }
                        }
                        
                        hasdbmsg=0;
                        break;
                        }
                    }
                    int i_getsel=atoi(c_getsel);
                    // cout<<"select:"<<i_getsel<<endl;
                    
                    if(i_getsel>=cmap[request_.course_id].capacity){
                        // string errmsg=" ";
                        string respos="{\"status\":\"error\",\"message\":\"该课程人数已满\"}";
                        // respos+=errmsg;
                        std::string dpath="/error.json";
                        response_.init(dir, dpath, request_.isKeepAlive(),"application/json",respos,403,2);

                    }
                    
                    else{
                    
                    
                    
                    // 建立客户端，创建套接字并绑定到KV服务器，然后发送命令
                    cout<<"与集群："<<s1_addr<<":"<<s1_port<<"建立连接"<<endl;
                    tcpc.setup(s1_addr,s1_port);                 
                    tcpc.Send(pac2);                  
                    tcpc.exit();
                    
                    while (1)
                    {
                        if(hasdbmsg==1){
                        
                        kb_getv1=msg;
                        hasdbmsg=0;
                        break;
                        }
                    }
                    
                    // cout<<"kb1:"<<kb_getv1<<"kb2:"<<kb_getv2<<endl;
                    int pos1=0;
                    
                    
                    string ischoose=request_.getdata_getV();
                    pos1=kb_getv1.find(ischoose);
                    // cout<<"pos1:"<<pos1<<endl;
                    

                    // int pos2=kb_getv2.find(ischoose);
                    if(pos1<0){
                    cout<<"与集群："<<s1_addr<<":"<<s1_port<<"建立连接"<<endl;
                    tcpc.setup(s1_addr,s1_port);                  
                    tcpc.Send(pac);                  
                    tcpc.exit();               
                    
                    //子线程接受并读取 KV服务器发出的信息
                    while (1)
                    {
                        if(hasdbmsg==1){
                        // int msglen=msg.length();
                        if(regex_match(msg,matchResult,  r)){
                            kb_getv1="ok";
                        }
                        else kb_getv1="error";
                        
                        // kb_getv1=msg.substr(1,msglen-2);
                        // kb_getv1=matchResult[1];
                        hasdbmsg=0;
                        break;
                        }
                    }

                    if(kb_getv1=="ok"){
                    string respos="{\"status\":\"";


                    int _selected=i_getsel+1;//选课人数+1
                    cmap[request_.course_id].selected=_selected;

                    // cout<<"_selected???:"<<
                    string key2="SET kb2",val2=to_string(_selected);
                    key2+=request_.course_id;
                    key2+=" ";
                    key2+=val2;
                    std::vector<std::string> listkb2;
                    split(key2, list, ' ');//SET A B
                    
                    std::string packb2 = p.getRESPArry(list);
                    // std::cout<<"post:"<<packb2<<std::endl;

                    // cout<<"与集群："<<s1_addr<<":"<<s1_port<<"建立连接"<<endl;
                    tcpc.setup(s2_addr,s2_port);                   
                    tcpc.Send(packb2);                  
                    tcpc.exit(); 
                    while (1)
                    {
                        if(hasdbmsg==1){
                        
                        hasdbmsg=0;
                        break;
                        }
                    }
                    

                    // else respos+="error";
                    respos+=kb_getv1;
                    respos+="\"}";
                    // cmap[request_.course_id].selected+=1;//选课人数+1
                    response_.init(dir, request_.path(), request_.isKeepAlive(),s,respos,200,2);
                    }
                    else{
                        string respos="{\"status\":\"error\",\"message\":\"操作失败\"}";
 
                        response_.init(dir, request_.path(), request_.isKeepAlive(),s,respos,403,2);

                    }
                    }
                    else{
                        string respos="{\"status\":\"ok\"}";
                        response_.init(dir, request_.path(), request_.isKeepAlive(),s,respos,200,2);

                    }

                    }
                    
                    }
                }
                else{
                    // string errmsg="";
                    string respos="{\"status\":\"error\",\"message\":\"payload 格式或数据非法\"}";
                    // respos+=errmsg;
                    std::string dpath="/error.json";
                    response_.init(dir, dpath, request_.isKeepAlive(),"application/json",respos,403,2);
                }

            }
            else if(request_.path()=="/api/drop"){
                if(request_.post_data_iserror==0){
					// std::cout << "dd:"<<request_.getdata()<<std::endl;
                    std::string s="application/json";
                    std::regex r(".*ok.*");

                    std::string tmsg=request_.getdata();

                    std::vector<std::string> list;
                    split(tmsg, list, ' ');//SET A B
                    Parser p;
                    std::string pac = p.getRESPArry(list);//把原始字符串转换成RESP形式
                    
                    if(cmap.find(request_.course_id)==cmap.end()){//没有该课程ID
                        string errmsg="\"没有该课程ID\"}";
                        string respos="{\"status\":\"error\",\"message\":";
                        respos+=errmsg;
                        std::string dpath="/error.json";
                        response_.init(dir, dpath, request_.isKeepAlive(),"application/json",respos,403,2);

                    }                   
                    else if(smap.find(request_.student_id)==smap.end()){//没有该学生ID
                        string errmsg="\"没有该学生ID\"}";
                        string respos="{\"status\":\"error\",\"message\":";
                        respos+=errmsg;
                        std::string dpath="/error.json";
                        response_.init(dir, dpath, request_.isKeepAlive(),"application/json",respos,403,2);

                    }
                    
                    else{
                    // TCPClient tcpc;
                    
                    //建立客户端，创建套接字并绑定到KV服务器，然后发送命令
                    tcpc.setup(s1_addr,s1_port);                   
                    tcpc.Send(pac);                  
                    tcpc.exit();
                    string respos="{\"status\":\"";
                    //子线程接受并读取 KV服务器发出的信息
                    while (1)
                    {
                        if(hasdbmsg==1){
                        if(regex_match(msg,matchResult,  r)){
                            kb_getv1="ok";
                        }
                        else kb_getv1="error";
                      
                        hasdbmsg=0;
                        break;
                        }
                    }

                    if(kb_getv1=="ok"){
                    string respos="{\"status\":\"";


                    int _selected=cmap[request_.course_id].selected-1;
                    string key2="SET kb2",val2=to_string(_selected);
                    key2+=request_.course_id;
                    key2+=" ";
                    key2+=val2;
                    std::vector<std::string> listkb2;
                    split(key2, list, ' ');//SET A B
                    
                    std::string packb2 = p.getRESPArry(list);
                    // std::cout<<"post:"<<packb2<<std::endl;

                    tcpc.setup(s2_addr,s2_port);                   
                    tcpc.Send(packb2);                  
                    tcpc.exit(); 
                    while (1)
                    {
                        if(hasdbmsg==1){
                        
                        hasdbmsg=0;
                        break;
                        }
                    }
                    cmap[request_.course_id].selected-=1;//选课人数+1

                    // else respos+="error";
                    respos+=kb_getv1;
                    respos+="\"}";
                    // cmap[request_.course_id].selected+=1;//选课人数+1
                    response_.init(dir, request_.path(), request_.isKeepAlive(),s,respos,200,2);
                    }
                    else{
                        string respos="{\"status\":\"error\",\"message\":\"操作失败\"}";
 
                        response_.init(dir, request_.path(), request_.isKeepAlive(),s,respos,200,2);

                    }
                    }
                    
                   
                }
                else{
                    // string errmsg="";
                    string respos="{\"status\":\"error\",\"message\":\"payload 格式或数据非法\"}";
                    // respos+=errmsg;
                    std::string dpath="/error.json";
                    response_.init(dir, dpath, request_.isKeepAlive(),"application/json",respos,403,2);
                }

            }
            else{
                response_.init(srcDir, request_.path(), request_.isKeepAlive(),"","", 404,1);
            }
        }
        else if(request_.method()=="GET"){
            // printf("get\n");
		std::smatch results;
		// std::cout<<request_.path()<<std::endl;
            if(request_.path()=="/api/check"){
                std::string dpath="/data.txt";
                response_.init(srcDir, dpath, request_.isKeepAlive(),"text/plain", "",200,1);
            }
            else if(request_.path()=="/api/list"){
                std::string dpath="/data.json";
                response_.init(srcDir, dpath, request_.isKeepAlive(),"application/json", "",200,1);
            }
            else if(std::regex_match(request_.path(),results, std::regex("/api/search.*"))){

            if(request_.path()=="/api/search/course?all"){
                string respos="{\"status\":\"ok\",\"data\":[";
                unordered_map<string, course>::iterator it,it2;
                int k=cmap.size(),k2=0;
                for(it=cmap.begin();it!=cmap.end();it++){
                    respos+="{\"id\":";
                    respos+=it->first;
                    respos+=",\"name\":\"";
                    course getc=it->second;
                    respos+=getc.name;
                    respos+="\",\"capacity\":";
                    respos+=to_string(getc.capacity);
                    respos+=",\"selected\":";
                    respos+=to_string(getc.selected);
                    k2+=1;
                    if(k2<k)respos+="},";
                    else respos+="}";
                }
                    respos+="]}";

					string dpath="/data.json";
					response_.init(srcDir, dpath, request_.isKeepAlive(),"application/json", respos,200,1);
            }
			else if (std::regex_match(request_.path(),results, std::regex("/api/search\\?id=([0-9]+)&name=([a-zA-Z0-9]+)"))){
					bool found = false;

					for(size_t i = 0; i < json_data.size(); i++)
						{		
								
							if (std::to_string(int(json_data[i]["id"]))==results.str(1) && std::string(json_data[i]["name"])==results.str(2))
							{
								found = true;
								// std::cout <<"SEC"<<std::endl;
								std::string res=std::string("[{\"id\":")+results.str(1)+std::string(",\"name\":\"")+results.str(2)+std::string("\"}]");
								response_.init(srcDir, res, request_.isKeepAlive(),"application/json", res,200,0);
								break;
							}
						}
					if (found== false){
								std::string dpath="/data.json";
								response_.init(srcDir, dpath, request_.isKeepAlive(),"application/json", "",200,1);
					}
			}
            else if (std::regex_match(request_.path(),results, std::regex("/api/search/course\\?id=([a-zA-Z0-9]+)"))){ //查找课程
                string get_id=results.str(1);
                
				if(cmap.find(get_id)==cmap.end()){//没有该课程ID
                        // string errmsg="";
                        string respos="{\"status\":\"error\",\"message\":\"没有该课程ID，无法查询到相关信息\"}";
                        // respos+=errmsg;
                        string dpath="/error.json";
                        response_.init(dir, dpath, request_.isKeepAlive(),"application/json",respos,403,2);

                }
				else{//查询到了
                    // //建立客户端，创建套接字并绑定到KV服务器，然后发送命令
                    string getsel="GET kb2";
                    getsel+=get_id;
                    std::vector<std::string> getsel_list;
                    split(getsel, getsel_list, ' ');
                    Parser p;
                    std::string getsel_pac = p.getRESPArry(getsel_list);

                    cout<<"与集群："<<s2_addr<<":"<<s2_port<<"建立连接"<<endl;
                    tcpc.setup(s2_addr,s2_port);                   
                    tcpc.Send(getsel_pac);                  
                    tcpc.exit();
                    char c_getsel[10];
                    int c_idx=0;
                    while (1)
                    {
                        if(hasdbmsg==1){
                        int len=msg.length();
                        int idx=0;
                        for(;idx<len;idx++){
                            if(msg[idx]=='/')break;
                        }
                        for(idx=idx+1;idx<len;idx++){
                            if(msg[idx]=='/')break;
                            else {
                                c_getsel[c_idx]=msg[idx];
                                c_idx+=1;
                            }
                        }
                        
                        hasdbmsg=0;
                        break;
                        }
                    }
                    // int i_getsel=atoi(c_getsel);
                    
                    string respos="{\"status\":\"ok\",\"data\":{\"id\":";
                    respos+=get_id;
                    respos+=",\"name\":\"";
                    course getc=cmap[get_id];
                    respos+=getc.name;
                    respos+="\",\"capacity\":";
                    respos+=to_string(getc.capacity);
                    respos+=",\"selected\":";
                    respos+=c_getsel;
                    respos+="}}";

					string dpath="/data.json";
					response_.init(srcDir, dpath, request_.isKeepAlive(),"application/json", respos,200,1);
				}

            }
            else if (std::regex_match(request_.path(),results, std::regex("/api/search/student\\?id=([0-9]+)"))){ 
                string get_id=results.str(1);
                
				if(smap.find(get_id)==smap.end()){//没有该课程ID
                        string errmsg="\"没有该学生ID，无法查询到相关信息\"}";
                        string respos="{\"status\":\"error\",\"message\":";
                        respos+=errmsg;
                        string dpath="/error.json";
                        response_.init(dir, dpath, request_.isKeepAlive(),"application/json",respos,403,2);

                }
				else{//查询到了
                    std::string tmsg="GET ";
                    tmsg+=get_id;
                    std::vector<std::string> list;
                    split(tmsg, list, ' ');//SET A B
                    Parser p;
                    std::string pac = p.getRESPArry(list);//把原始字符串转换成RESP形式
                    
                    // TCPClient tcpc;                    
                    //建立客户端，创建套接字并绑定到KV服务器，然后发送命令
                    tcpc.setup(s1_addr,s1_port);                  
                    tcpc.Send(pac);                  
                    tcpc.exit();
                    string isfind="";
                    //子线程接受并读取 KV服务器发出的信息
                    while (1)
                    {
                        if(hasdbmsg==1){
                        isfind=msg;
                        hasdbmsg=0;
                        break;
                        }
                    }
                    
                    
                    string respos="{\"status\":\"ok\",\"data\":{\"id\":";
                    respos+=get_id;
                    respos+=",\"name\":\"";
                    respos+=smap[get_id];
                    // cout<<"==="<<isfind<<endl;
                    std::regex r2(".*error_find.*$");
                    if(regex_match(isfind,matchResult,  r2)){
                            respos+="\",\"courses\":[]}}";
                    }
                    else{
                    int isfindlen=isfind.length();
                    respos+="\",\"courses\":[";
                    string tid="";
                    for(int i=1;i<isfindlen-1;i++){
                        
                        if(isfind[i]==','){
                            respos+="{\"id\"=";
                            respos+=tid;
                            respos+=",\"name\":\"";
                            respos+=cmap[tid].name;
                            respos+="\"}";
                            if(i<isfindlen-2)respos+=",";
                            tid="";


                        }
                        else tid+=isfind[i];
                    
                    
                    }
                    respos+="]}}";

                    }
                    

					string dpath="/data.json";
					response_.init(srcDir, dpath, request_.isKeepAlive(),"application/json", respos,200,1);
				}

            }
            else {
                string respos="{\"status\":\"error\",\"message\":\"query string 格式或数据非法\"}";
                
                string dpath="/error.json";
                response_.init(dir, dpath, request_.isKeepAlive(),"application/json",respos,403,2);

            }
            }
            else{
            response_.init(srcDir, request_.path(), false,"", "",200,1);
            }
        }
		else{
			response_.init(srcDir, request_.path(), request_.isKeepAlive(),"","", 501,1);
		}

	}
    response_.makeResponse(writeBuffer_);
    /* 响应头 */
    iov_[0].iov_base = const_cast<char*>(writeBuffer_.curReadPtr());
    iov_[0].iov_len = writeBuffer_.readableBytes();
    iovCnt_ = 1;

    /* 文件 */
    if(response_.fileLen() > 0  && response_.file()) {
        iov_[1].iov_base = response_.file();
        iov_[1].iov_len = response_.fileLen();
        iovCnt_ = 2;
    }
    return true;
}