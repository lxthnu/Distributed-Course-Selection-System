//#include "../include/core.hpp"
#include "../include/participant.h"
#include "../include/parser.hpp"
#include <string.h>
using namespace std;

int kv_debug=0;

sem_t Participant::psem[5];
mutex Participant::mut[10];

std::thread Participant::beginAccpet(){
   return std::move(RUS.beginAccept());
}


void Participant::debug(){
    RUS.debug();
}


int Participant::msgSend(NodeInfo node,string msg){
    RUS.TCPCclose();
    while(RUS.RUsend(node,msg,100000)!=0){
        cerr << "[ERROR]send msg faild <part.cpp 25>\n";
        return ERROR;
    }
    RUS.TCPCclose();
    return OK;
}

/*
the func can just send the heartbeat, it's possible that
you may not receive any msg
*/
void* Participant::sendHeartBeat(void* arg){//这里要修改，一次性发完
    int l=follower_info_list.size();
    // bool flag=false;
    while(1){
        // sem_wait(&psem[2]);
        while(!msgSendQueue.empty()){
            auto it=msgSendQueue.front();
            msgSendQueue.pop();
            if(kv_debug==1)printf("--------------------------------------send %s\n",it.second.c_str());
            

            if(it.second.find("[HEART]")!=string::npos && pstate!=LEADER) continue;
            
            if(msgSend(it.first,it.second)!=0){
                if(kv_debug==1)printf("part.49 send fail \n");
            }
        }
        if(pstate==LEADER){
            heartAll();
        }
        usleep(1000000);
    }
    return NULL;
}

void Participant::rollBackAll(){
    int l=follower_info_list.size();
    string posi,index,state,ans,add;
    int port;
    add=MY.add;port=MY.port;
    posi=to_string(log.getNewposi());
    index=to_string(log.getNewLogIndex());
    for(int i=0;i<l;i++){
        ans="[ROLL]/add:"+add+"/port:"+to_string(port)+"/posi:"+posi+"/index:"+index+"/res:"+to_string(round)+"/";
        msgSendQueue.push(make_pair(follower_info_list[i],ans));
        sem_post(&psem[2]);
    }
    
}

void Participant::commitAll(){
    int l=follower_info_list.size();
    string posi,index,state,ans,add;
    int port;
    add=MY.add;port=MY.port;
    posi=to_string(log.getNewposi());
    index=to_string(log.getNewLogIndex());
    for(int i=0;i<l;i++){
        ans="[COM]/add:"+add+"/port:"+to_string(port)+"/posi:"+posi+"/index:"+index+"/res:"+to_string(round)+"/";
        msgSendQueue.push(make_pair(follower_info_list[i],ans));
        sem_post(&psem[2]);
    }
    
}

void Participant::prepareAll(string op,string val){
    int l=follower_info_list.size();
    string posi,index,state,ans,add;
    int port;
    add=MY.add;port=MY.port;
    posi=to_string(log.getNewposi());
    index=to_string(log.getNewLogIndex());
    for(int i=0;i<l;i++){
        ans="[PRE]/add:"+add+"/port:"+to_string(port)+"/posi:"+posi+"/index:"+index+"/op:"+op+"/res:"+val+"/";
        msgSendQueue.push(make_pair(follower_info_list[i],ans));
        sem_post(&psem[2]);
    }
    
}

void Participant::heartAll(){
    int l=follower_info_list.size();
    string posi,index,state,ans,add;
    int port;
    add=MY.add;port=MY.port;
    posi=to_string(log.getNewposi());
    index=to_string(log.getNewLogIndex());
    for(int i=0;i<l;i++){
        ans="[HEART]/add:"+add+"/port:"+to_string(port)+"/posi:"+posi+"/index:"+index+"/res:"+to_string(round)+"/";
        msgSendQueue.push(make_pair(follower_info_list[i],ans));
        sem_post(&psem[2]);
    }
    
}

// enum DATASTATE Participant::transState(string s){
//     if(s=="0") return LOG_COMMIT;
//     else if(s=="1") return LOG_PRE;
//     else if(s=="2") return INVALID;
//     else if(s=="3") return LOG_ABORT;
// }

int Participant::performComm(){
    Command com;
    string index,posi,state,ans,add;
    int port;
    add=MY.add;port=MY.port;
    NodeInfo des;
    while(1){
        ans="";
        sem_wait(&psem[1]);
        if(kv_debug==1)printf("--------------------part.103\n");
        bool flag=false;
        if(!comQueue.empty()){
            com=comQueue.front();
            if(com.type!="[VOTE]" ||com.type!="[VOTERE]" ) timeOut=0;
            comQueue.pop();
            des.add=com.add,des.port=atoi(com.port.c_str());
            if(com.type=="[CLIENT]"){
                if(pstate!=LEADER){
                    des.add=LEA.add,des.port=LEA.port;
                    ans=com.reCmd;
                    if(kv_debug==1)cout << "EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE " << ans << endl;
                    goto LASTL;
                }else{
                    Client.add=com.add;Client.port=8010;
                    log.writeLog(log.createLog(round,com.op,com.res));
                    dataState=LOG_PRE;

                    prepareAll(com.op,com.res);
                    // std::thread ppre(&Participant::preTimer,this,5);
                    // ppre.detach();
                    
                    continue;
                }
            }
            else if(com.type=="[VOTE]"){
                if(kv_debug==1)printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ %d %d\n",atoi(com.res.c_str()),round);
                if(atoi(com.res.c_str())>=round){
                    if(kv_debug==1)printf("%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% switch to follower\n");
                    if(atoi(com.res.c_str())>round){
                        pstate=FOLLOWER;
                        // sleep(10);
                    }
                    // mut[0].lock();
                    // for(int i=0;i<50;i++){
                    //     cout << "old:" << round<< endl;
                    // }
                    round=atoi(com.res.c_str());
                    // for(int i=0;i<50;i++){
                    //     cout << "new:" << round<< endl;
                    // }
                    timeOut=0;
                    voteNum=1;
                    // mut[0].unlock();
                    flag=true;
                }
                else flag=false;
                posi=to_string(log.getNewposi());
                index=to_string(log.getNewLogIndex());
                if(flag){
                    cout << "part.161\n"; 
                    ans="[VOTERE]/add:"+add+"/port:"+to_string(port)+"/posi:"+posi+"/index:"+index+"/res:"+to_string(round)+"/";
                }
            }else if(com.type=="[VOTERE]" && pstate==CANDIDATE && atoi(com.res.c_str())==round){
                for(int i=0;i<10;i++){
                        cout << voteNum << endl;
                        cout << "==============================================================================================================\n";
                    }
                flag=false;
                voteNum++;
                if(voteNum>=memberNum/2+1){
                    for(int i=0;i<50;i++){
                        cout << voteNum << endl;
                        cout << "==============================================================================================================\n";
                    }
                    voteNum=1;
                    pstate=LEADER;
                    heartAll();
                }
                continue;
            }
            else if(pstate==FOLLOWER || pstate==CANDIDATE || pstate==LEADER){//if the node'state is follower
                if(kv_debug==1)printf("--------------------part.113\n");
                if(com.type=="[HEART]"){
                    if(com.res.find("{")==string::npos)
                        if(atoi(com.res.c_str())>=round && atoi(com.pos.c_str())>=log.getNewposi() && atoi(com.index.c_str())>=log.getNewLogIndex()){
                            pstate=FOLLOWER;
                            timeOut=0;
                            round=atoi(com.res.c_str());
                            LEA.add=com.add;
                            LEA.port=atoi(com.port.c_str());
                        }else{
                            continue;
                        }
                    // for(int i=0;i<500;i++)
                    //     cout << "198 ============= folower\n";
                    if(kv_debug==1)printf("--------------------part.115\n");
                    if(dataState!=INVALID && com.pos==to_string(log.getNewposi()) && com.index==to_string(log.getNewLogIndex())){
                        flag=true;
                        string s=log.getState();
                        if(s=="0" || s=="~") dataState=LOG_COMMIT;
                        else if(s=="1") dataState=LOG_PRE;
                        else if(s=="2") dataState=INVALID;//恢复状态
                        else if(s=="3") dataState=LOG_ABORT;
                        if(kv_debug==1)printf("--------------------part.120\n");
                    }
                    else if(dataState!=INVALID && (com.pos!=to_string(log.getNewposi()) || com.index!=to_string(log.getNewLogIndex()))){
                        if(kv_debug==1)printf("--------------------part.125\n");
                        flag=true;
                        dataState=INVALID;//进入恢复阶段
                        printf("[WORNING] data does not fit and get into recover <part.cpp 68>\n");
                    }else if(dataState==INVALID){
                        if(kv_debug==1)printf("--------------------part.129\n");         
                        flag=true;
                        if(com.pos==to_string(log.getNewposi()) && com.index==to_string(log.getNewLogIndex()) && com.res.size()<20){
                            dataState=LOG_ABORT;
                            if(kv_debug==1)printf("[WORNING] recover finish! <part.cpp 216>\n");
                            continue;
                        }
                        if(com.res.empty() || com.res.size()<10){
                            //flag=false;
                            if(kv_debug==1)printf("waiting for the recover heart beat... <part.cpp 76>\n");
                            ans="[HEARTRE]";
                            goto FINALL;
                            //continue;
                        }
                        if(kv_debug==1) printf("[WORNING] recover.... <part.cpp 223>\n");
                        string op,val,oldval,_state;
                        P.reComPar(com.res,op,val,oldval,_state);
                        if(kv_debug==1)cout << "###### " << op << ' ' << val << ' ' << oldval << ' ' << _state << endl;
                        if(atoi(com.pos.c_str())>=log.getNewposi()){
                            Log l_=log.createLog(atoi(com.index.c_str()),op,val,oldval);
                            if(log.writeLog(l_)!=false){
                                if(kv_debug==1)printf("--------------------part.225\n");    
                                if(_state=="0"){//commit state
                                    if(kv_debug==1)printf("--------------------part.227\n");    
                                    string ts;
                                    flag=log.commitLog(ts);
                                    cout << "[part 260 result] " << log.getLog(log.getNewposi()-1).op.operation << ' ' << log.getLog(log.getNewposi()-1).op.value << endl; 
                                    if(!flag)
                                        if(kv_debug==1)printf("[ERROR] recver or commit wrong <part.cpp 285>\n");
                                }
                            }
                        }
                    }
                    ans="[HEARTRE]";
                }else if(com.type=="[PRE]"){
                    flag=log.writeLog(log.createLog(atoi(com.index.c_str()),com.op,com.res));
                    posi=to_string(log.getNewposi());
                    index=to_string(log.getNewLogIndex());
                    ans="[PRERE]";
                    if(flag){
                        dataState=LOG_PRE;
                        ans+="/add:"+add+"/port:"+to_string(port)+"/posi:"+posi+"/index:"+index+"/res:ok/";
                    }else{
                        ans+="/add:"+add+"/port:"+to_string(port)+"/posi:"+posi+"/index:"+index+"/res:error/";
                    }
                    goto LASTL;
                }else if(com.type=="[ROLL]"){
                    flag=log.rollbackLog();
                    posi=to_string(log.getNewposi());
                    index=to_string(log.getNewLogIndex());
                    ans="[ROLLRE]";
                    if(flag){
                        dataState=LOG_ABORT;
                        ans+="/add:"+add+"/port:"+to_string(port)+"/posi:"+posi+"/index:"+index+"/res:ok/";
                    }else{
                        ans+="/add:"+add+"/port:"+to_string(port)+"/posi:"+posi+"/index:"+index+"/res:error/";
                    }
                    goto LASTL;
                }else if(com.type=="[COM]"){
                    string ts;
                    flag=log.commitLog(ts);
                    posi=to_string(log.getNewposi());
                    index=to_string(log.getNewLogIndex());
                    ans="[COMRE]";
                    if(flag){
                        dataState=LOG_ABORT;
                        ans+="/add:"+add+"/port:"+to_string(port)+"/posi:"+posi+"/index:"+index+"/res:ok/";
                    }else{
                        ans+="/add:"+add+"/port:"+to_string(port)+"/posi:"+posi+"/index:"+index+"/res:error/";
                    }
                    goto LASTL;
                }else{
                    printf("[WARNING] type not fit? <part.cpp 148>\n");
                    flag=false;
                }
                FINALL:
                if(kv_debug==1)printf("--------------------part.152\n");
                posi=to_string(log.getNewposi());
                index=to_string(log.getNewLogIndex());
                if(flag){
                    ans+="/add:"+add+"/port:"+to_string(port)+"/posi:"+posi+"/index:"+index+"/res:"+to_string(round)+"/";
                }
            }
            if(pstate==LEADER){
                if(com.type=="[HEARTRE]"){
                    if(kv_debug==1)printf("--------------------part.257\n");
                    if(com.index==to_string(log.getNewLogIndex()) && com.pos==to_string(log.getNewposi())){
                        flag=true;
                        posi=to_string(atoi(com.pos.c_str()));
                        index=to_string(log.getNewLogIndex());
                        continue;
                        ans="[HEART]/add:"+add+"/port:"+to_string(port)+"/posi:"+posi+"/index:"+index+"/res:"+to_string(round)+"/";
                    }else{
                        flag=true;
                        if(kv_debug==1)printf("--------------------part.265\n");
                        if(atoi(com.pos.c_str())!=log.getNewposi()){
                            if(kv_debug==1)printf("--------------------part.267\n");
                            posi=to_string(atoi(com.pos.c_str()));
                            Log l_=log.getLog(atoi(posi.c_str()));
                            index=to_string(l_.LogIndex);
                            ans="[HEART]/add:"+add+"/port:"+to_string(port)+"/posi:"+posi+"/index:"+index+"/res:{op:"+l_.op.operation+"val:"+l_.op.value+"oldval:"+l_.op.oldVal+"state:"+l_.getState()+"}"+"/";
                        }
                    }
                }else if(com.type=="[PRERE]"){
                    if(kv_debug==1)printf("--------------------part.293\n");
                    if(com.res=="ok"){
                        if(kv_debug==1)printf("--------------------part.295\n");
                        succNumP++;
                        if(succNumP>=memberNum/2+1){
                            // ans="[CLENTRE]/res:ok";//raft算法修改为二阶段
                            dataState=LOG_COMMIT;
                            commitAll();
                            succNumP=0;failNumP=0;
                            continue;
                        }
                    }else{
                        if(kv_debug==1)printf("--------------------part.303\n");
                        failNumP++;
                        if(failNumP>=memberNum/2+1){
                            if(kv_debug==1)printf("--------------------part.306\n");
                            des.add=Client.add,des.port=Client.port;
                            // ans="[CLENTRE]/add:"+add+"/port:"+to_string(port)+"/res:error/";
                            ans="/error/";
                            failNumP=0;succNumP=0;
                            rollBackAll();
                            log.rollbackLog();
                            continue;
                        }
                    }
                }else if(com.type=="[ROLLRE]"){
                    dataState=LOG_ABORT;//鲁棒性
                    if(com.res=="ok"){
                        flag=true;
                    }else{
                        printf("roll back fail <pact.cpp 170>\n");
                        flag=false;
                    }
                }else if(com.type=="[COMRE]"){
                    if(kv_debug==1)printf("--------------------part.348\n");
                    if(com.res=="ok"){
                        // cout << "[1221]"+com.port +"/"+com.res << endl;
                        if(kv_debug==1)printf("--------------------part.350\n");
                        succNumC++;
                        if(succNumC>=memberNum/2+1){
                            dataState=LOG_ABORT;
                            if(kv_debug==1)printf("--------------------part.355\n");
                            string ts;
                            des.add=Client.add,des.port=Client.port;
                            int turnWhile=0;
                            while(++turnWhile<10){
                                if(log.commitLog(ts)){
                                    if(!ts.empty()){
                                        // ans="[CLENTRE]/add:"+add+"/port:"+to_string(port)+"/res:ok"+"/res:"+ts+"/";
                                        // ans="ok  res:";
                                        ans="/";
                                        ans+=ts;
                                        ans+="/";
                                     }
                                    else{
                                        // cout << "[122]" << endl;
                                        // ans="[CLENTRE]/add:"+add+"/port:"+to_string(port)+"/res:ok/";
                                        ans="/ok/";
                                    }  
                                    break;
                                }
                            }
                            if(turnWhile>=10) ans="[CLENTRE~]/add:"+add+"/port:"+to_string(port)+"/res:error/";
                            succNumC=0;failNumC=0;
                        }
                    }
                    else{
                        failNumC++;
                        if(failNumC>=memberNum/2+1){
                            dataState=LOG_ABORT;
                            des.add=Client.add,des.port=Client.port;
                            // ans="[CLENTRE]/add:"+add+"/port:"+to_string(port)+"/res:error_find/";
                            ans="/error_find/";
                            failNumC=0;succNumC=0;
                            rollBackAll();
                            log.rollbackLog();
                        }
                    }
                }
                else{
                    printf("[ERROR] type: %s not fit? <part.cpp 266>\n",com.type.c_str());
                    continue;
                }
            }
            LASTL:
            if(kv_debug==1)printf("-----------------ans: %s\n",ans.c_str());
            if(!ans.empty()){
                msgSendQueue.push(make_pair(des,ans));
                sem_post(&psem[2]);
            }
        }
    }
    return OK;
}

int Participant::PCommandParser(){
    string cmd;
    Command com;
    while(1){
        if(kv_debug==1)printf("--------------------part.422\n");
        sem_wait(&psem[0]);
        if(kv_debug==1)printf("--------------------part.424\n");
        if(!cmdQueue.empty()){
            if(kv_debug==1)printf("--------------------part.426\n");
            cmd=cmdQueue.front();
            if(kv_debug==1)printf("%s\n",cmd.c_str());
            if(kv_debug==1)printf("--------------------part.428\n");
            cmdQueue.pop();
            if(kv_debug==1)printf("--------------------part.431\n");
            com=P.commandParser(cmd);
            if(com.type=="[CLIENT]")
                com.reCmd=cmd;
            // if(com.type=="[HEART]"){
            //     for(int i=0;i<10000;i++)
            //         cout << "why!!!\n";
            // }
            if(kv_debug==1)printf("--------------------part.439\n");
            comQueue.push(com);
            sem_post(&psem[1]);
        }
    }
    if(kv_debug==1)printf("--------------------part.444\n");
    return OK;
}

int Participant::getCommand(){//need 
    string cmd;
    while(1){
        if(RUS.getCommand(cmd)){
            if(kv_debug==1)printf("--------------------receive cmd:%s\n",cmd.c_str());
            cmdQueue.push(cmd);
            sem_post(&psem[0]);
        }
    }
    return ERROR;
}

int Participant::pSend(string dest,string msg){

    return OK;
}

int Participant::Init(std::vector<NodeInfo> ps,NodeInfo node){
    int len=ps.size();
    MY=node;
    memberNum=len+1;
    pstate=FOLLOWER;
    voteNum=1;
    //c绑定0.0.0.0,ps按各自分配的地址作为绑定地址
    if(RUS.RURecvConnection(node,TIMEOUT)!=0){
        cerr << "[---ERROR---] init failed <parti.cpp 306>\n";
    }
    for(int i=0;i<len;i++){
        follower_info_list.push_back(ps[i]);
    }
        
    return 0;
}

void Participant::Timer(int t){
    while(1){
        if(pstate==LEADER){
            sleep(t);
            // sem_post(&psem[4]);
            continue;
        }
        int tk=(rand()%2==1)?3:5;
        cout << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% " <<round << endl; 
        if(timeOut>=tk){
            pstate=CANDIDATE;
            mut[0].lock();
            round++;
            timeOut=0;
            voteNum=1;
            mut[0].unlock();
            sem_post(&psem[4]);
        }
        sleep(t);
        timeOut++;
    }
}

void Participant::preTimer(int t){
    string ans,add;
    int port;
    add=MY.add;port=MY.port;
    for(int i=0;i<t;i++){
        if(dataState!=LOG_PRE) return;
        sleep(1);
    }
    if(dataState!=LOG_PRE) return;
    if(succNumP<memberNum/2+1){
        rollBackAll();
        ans="[CLIENT]/add:"+add+"/port:"+to_string(port)+"/res:error/";
        msgSendQueue.push(make_pair(Client,ans));
        succNumP=0;
        dataState=LOG_ABORT;
    }
}

void Participant::comTimer(int t){
    string ans,add;
    int port;
    add=MY.add;port=MY.port;
    for(int i=0;i<t;i++){
        if(dataState!=LOG_COMMIT) return;
        sleep(1);
    }
    if(dataState!=LOG_COMMIT) return;
    if(succNumC<memberNum/2+1){
        rollBackAll();
        ans="[CLIENT]/add:"+add+"/port:"+to_string(port)+"/res:error/";
        msgSendQueue.push(make_pair(Client,ans));
        succNumC=0;
        dataState=LOG_ABORT;
    }
}

void Participant::voteAll(){
    while(1){
        sem_wait(&psem[4]);
        if(pstate!=CANDIDATE) continue;
        if(kv_debug==1)printf("===============================new candidate of %d begin===============================\n",round);
        int l=follower_info_list.size(),port;
        string add,posi,index,ans;
        NodeInfo des;
        posi=to_string(log.getNewposi());
        index=to_string(log.getNewLogIndex());
        for(int i=0;i<l;i++){
            des.add=follower_info_list[i].add;
            des.port=follower_info_list[i].port;
            ans="[VOTE]/add:"+MY.add+"/port:"+to_string(MY.port)+"/posi:"+posi+"/index:"+index+"/res:"+to_string(round)+"/";
            msgSendQueue.push(make_pair(des,ans));
            sem_post(&psem[2]);
        }
    }
}

void Participant::Working(){
    if(MY.port==8001){
        pstate=LEADER;
    }
    //     log.writeLog(log.createLog(1,"SET","LHZ,HERO"));
    //     string ts;
    //     log.commitLog(ts);
    //     printf("--------------------part.543\n");
    //     string cmd="[client]/op:GET/res:LHZ/";
    //     cmdQueue.push(cmd);
        
    // }else if(MY.port==8002){
    //     pstate=FOLLOWER;
    //     log.writeLog(log.createLog(1,"SET","LHZ,HERO"));
    //     string ts;
    //     log.commitLog(ts);
    // }

    std::thread pAccept=beginAccpet();//消息放入Message队列
    if(kv_debug==1)printf("--------------------1\n");
    std::thread pHandle=RUS.handle();//从Message取信息放入message
    if(kv_debug==1)printf("--------------------2\n");
    std::thread pGetCommand(&Participant::getCommand,this);//放入cmd队列
    if(kv_debug==1)printf("--------------------3\n");
    std::thread pParseCom(&Participant::PCommandParser,this);//从cmd取信息解析后放入com队列
    if(kv_debug==1)printf("--------------------4\n");
    std::thread pPerformCom(&Participant::performComm,this);//从com队列取命令并实施
    if(kv_debug==1)printf("--------------------5\n");
    std::thread pSend(&Participant::sendHeartBeat,this,(void*)0);//发送回应
    if(kv_debug==1)printf("--------------------6\n");
    std::thread pTimer1(&Participant::Timer,this,1);//follower超时选举
    if(kv_debug==1)printf("--------------------7\n");
    std::thread pVote(&Participant::voteAll,this);//follower超时发送选票
    if(kv_debug==1)printf("--------------------8\n");

    

    pHandle.join();
    pAccept.join();
    pGetCommand.join();
    pParseCom.join();
    pPerformCom.join();
    pSend.join();
    pVote.join();
    pTimer1.join();
}

// void Participant::test(){
//     std::thread tHandle=RUS.handle();
//     descript_socket* Info;
//     while(1){
//         if(RUS.getCommand(Info)){
//             cerr << Info->message<< endl;
//             RUS.TCPclose(Info);
//         }
//     }
//     tHandle.join();
// }

Participant::Participant(){
    round=0,voteNum=1,succNumP=0,succNumC=0,failNumC=0,failNumP=0,memberNum=0,timeOut=0;
    for(int i=0;i<5;i++){
        if (sem_init(&psem[i],0,0)) {
            printf("[ERROR] Semaphore initialization failed!! <parti.cpp 118>\n");
            exit(EXIT_FAILURE);
        }
    }
}

int Participant::getState(){
    return pstate;
}

