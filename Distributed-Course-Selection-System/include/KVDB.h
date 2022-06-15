//
// Created by Rem_Hero on 2021/5/31.
//

#ifndef SHAREFILE_KVDB_H
#define SHAREFILE_KVDB_H
#include <map>
#include <vector>
#include <string.h>
#include <iostream>
using namespace std;

//数据库在内存中，以map为基础
class KVDB{
    private:
        enum{OK,ERROR};
        map<string,map<string,int>> DB;
        map<string,string> DB2;
    public:
        KVDB();
        int setValue(string key,string value,string &oldval);
        int delValue(string key,string value,string &oldval);
        int getValue(string key,string &value);
};


#endif //SHAREFILE_KVDB_H
