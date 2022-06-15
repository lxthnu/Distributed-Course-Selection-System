# 分布式选课系统
![1](https://user-images.githubusercontent.com/58248490/173798767-7115d350-985d-477e-bf59-e6af9af40afb.png)



## 编译

```
#在Distributed-Course-Selection-System文件夹下
make
```


### 启动Store Server
在不同终端依次运行下列命令
```
#启动存储服务器集群1（该集群主要存储 学生-选课 数据）
..$ ./CS_system --kvconfig_path ./conf/coordinator.conf
..$ ./CS_system --kvconfig_path ./conf/coordinator1.conf
..$ ./CS_system --kvconfig_path ./conf/coordinator2.conf
..$ ./CS_system --kvconfig_path ./conf/coordinator3.conf

#启动存储服务器集群2（该集群主要存储 课程-选课人数 数据）
..$ ./CS_system --kvconfig_path ./conf/tcoordinator.conf
..$ ./CS_system --kvconfig_path ./conf/tcoordinator1.conf
..$ ./CS_system --kvconfig_path ./conf/tcoordinator2.conf
..$ ./CS_system --kvconfig_path ./conf/tcoordinator3.conf
```

### 启动web服务器
新建终端，运行下列命令
```
..$ ./CS_system --ip 127.0.0.1 --port 8080 --config_path ./conf/store_servers.conf
```


### 客户端命令
新建终端，运行下列命令

#### get
```
1、获取某项课程信息：
..$ curl -i -G -d 'id=AR03015' -X GET http://localhost:8080/api/search/course
成功时返回：
HTTP/1.1 200 OK
Content-type: application/json
Content-length: 88

{"status":"ok","data":{"id":AR03015,"name":"中国建筑史","capacity":3,"selected":0}}

我们提供以下几种错误提示信息：
（1）query string 格式或数据非法：
..$ curl -i -G -d '？？=AR03015' -X GET http://localhost:8080/api/search/course
返回：
HTTP/1.1 403 Forbidden
Content-type: application/json
Content-length: 65

{"status":"error","message":"query string 格式或数据非法"}

（2）无法查询到相关信息：
..$ curl -i -G -d 'id=A' -X GET http://localhost:8080/api/search/course
返回：
HTTP/1.1 403 Forbidden
Content-type: application/json
Content-length: 78

{"status":"error","message":"没有该课程ID，无法查询到相关信息"}


2、获取所有课程信息:
..$ curl -i -G -d 'all'  -X GET http://localhost:8080/api/search/course
返回：
HTTP/1.1 200 OK
Content-type: application/json
Content-length: 5666

{"status":"ok","data":[{"id":BA05128,"name":"审计学","capacity":56,"selected":0},{"id":BA05127,"name":"国际会计学（S）","capacity":24,"selected":0},{"id":BA05146,"name":"信息网络工程设计","capacity":33,"selected":0},{"id":BA05126,"name":"财务分析","capacity":15,"selected":0},...,]}


3、获取某个学生的所选课程:
..$ curl -i -G -d 'id=202208010103' -X GET http://localhost:8080/api/search/student
成功时返回：
HTTP/1.1 200 OK
Content-type: application/json
Content-length: 149

{"status":"ok","data":{"id":202208010103,"name":"学生3","courses":[{"id"=AR03015,"name":"中国建筑史"},{"id"=AR03024,"name":"专业美术I"}]}}

我们提供以下几种错误提示信息：
（1）query string 格式或数据非法：
..$ curl -i -G -d '？？=202208010105' -X GET http://localhost:8080/api/search/student
返回：
HTTP/1.1 403 Forbidden
Content-type: application/json
Content-length: 65

{"status":"error","message":"query string 格式或数据非法"}

（2）无法查询到相关信息：
..$ curl -i -G -d 'id=2' -X GET http://localhost:8080/api/search/student
返回：
HTTP/1.1 403 Forbidden
Content-type: application/json
Content-length: 78

{"status":"error","message":"没有该学生ID，无法查询到相关信息"}



```

#### post
```
1、学生进行选课：
..$ curl -i -d '{"student_id":202208010103,"course_id":AR03015}'  -H 'Content-Type: application/json' -X POST http://localhost:8080/api/choose
成功时返回：
HTTP/1.1 200 OK
Content-type: application/json
Content-length: 15

{"status":"ok"}


我们提供以下几种错误提示信息：
（1）payload 格式或数据非法：
..$ curl -i -d '{"student_id":202208010101,"？？":AR03015}'  -H 'Content-Type: application/json' -X POST http://localhost:8080/api/choose
HTTP/1.1 403 Forbidden
Content-type: application/json
Content-length: 60

{"status":"error","message":"payload 格式或数据非法"}

（2）无法查询到相关信息： 
..$ curl -i -d '{"student_id":1,"course_id":AR03015}'  -H 'Content-Type: application/json' -X POST http://localhost:8080/api/choose
返回：
HTTP/1.1 403 Forbidden
Content-type: application/json
Content-length: 48

{"status":"error","message":"没有该学生ID"}

..$ curl -i -d '{"student_id":202208010101,"course_id":1}'  -H 'Content-Type: application/json' -X POST http://localhost:8080/api/choose
返回：
HTTP/1.1 403 Forbidden
Content-type: application/json
Content-length: 48

{"status":"error","message":"没有该课程ID"}

（3）课程已满：
返回：
HTTP/1.1 403 Forbidden
Content-type: application/json
Content-length: 52

{"status":"error","message":"该课程人数已满"}



2、学生进行退课：
..$ curl -i -d '{"student_id":202208010101,"course_id":AR03015}'  -H 'Content-Type: application/json' -X POST http://localhost:8080/api/drop
成功时返回：
HTTP/1.1 200 OK
Content-type: application/json
Content-length: 15

{"status":"ok"}


我们提供以下几种错误提示信息：
（1）payload 格式或数据非法：
..$ curl -i -d '{"student_id":202208010101,"??":AR03015}'  -H 'Content-Type: application/json' -X POST http://localhost:8080/api/drop
HTTP/1.1 403 Forbidden
Content-type: application/json
Content-length: 60

{"status":"error","message":"payload 格式或数据非法"}

（2）无法查询到相关信息： 
..$ curl -i -d '{"student_id":1,"course_id":AR03015}'  -H 'Content-Type: application/json' -X POST http://localhost:8080/api/drop
返回：
HTTP/1.1 403 Forbidden
Content-type: application/json
Content-length: 48

{"status":"error","message":"没有该学生ID"}

..$ curl -i -d '{"student_id":202208010101,"course_id":1}'  -H 'Content-Type: application/json' -X POST http://localhost:8080/api/drop
返回：
HTTP/1.1 403 Forbidden
Content-type: application/json
Content-length: 48

{"status":"error","message":"没有该课程ID"}


```
