//
// Created by Rem_Hero on 2021/5/31.
//

#include "../include/KVDB.h"
#include <unistd.h>
using namespace std;

KVDB::KVDB(){

}

int KVDB::setValue(string key,string value,string &oldval){
    oldval="";
    if(key[0]=='k'&&key[1]=='b'&&key[2]=='2'){
        oldval=DB2[key];
        DB2[key]=value;

    }
    else{
    map<string, int>::iterator iter;
        iter = DB[key].begin();
        while(iter != DB[key].end()){
	    oldval += iter->first;	
        oldval+=",";    
	    iter++;
        }

    DB[key][value]=1;
    }
    // printf("kv:set ok\n");
    // sleep(1);
    return OK;
}

int KVDB::delValue(string key,string value,string &oldval){
    oldval="";
    if(key[0]=='k'&&key[1]=='b'&&key[2]=='2'){
        oldval=DB2[key];
        DB2.erase(key);

    }
    else{
    map<string, int>::iterator iter;
        iter = DB[key].begin();
        while(iter != DB[key].end()){
	    oldval += iter->first;	
        oldval+=",";    
	    iter++;
        }
    DB[key].erase(value);
    
    }
    return OK;
}

int KVDB::getValue(string key,string &value){
    if(key[0]=='k'&&key[1]=='b'&&key[2]=='2'){
        if(DB2.find(key)!=DB2.end())
            value=DB2[key];
        else value="0";
        return OK;


    }
    else{
    if(DB.find(key)!=DB.end()){
        cout << "[DB33]get ok\n";
        // map<string, int> t;
        // t=DB[key];
        value="";
        map<string, int>::iterator iter;
        iter = DB[key].begin();
        while(iter != DB[key].end()){
	    value += iter->first;	
        value+=",";    
	    iter++;
        }
        return OK;
    }  
    return ERROR;
    }
}
