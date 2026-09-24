# cpp_vec_server_db1

 Version: 0.9.3

 date    : 2026/09/23
 
 update :

***

C++ Vector DB Server HTTP , memory database SQLite

* LLVM CLang
* cpp-httplib
* sqlite3 use

***
### related Client

* http client , RAG app

https://github.com/kuc-arc-f/cpp_16ex/tree/main/vec_cl_1

***
### related

https://github.com/yhirose/cpp-httplib

***
* LIB add
```
sudo apt update
sudo apt-get install libsqlite3-dev
sudo apt-get install nlohmann-json3-dev
sudo apt install libspdlog-dev libfmt-dev
```

***
* table add
```
sqlite3 ./data/backup.db < table.sql
```

***
* build
```
make all
```

* start , localhost:8888

```
./start.sh
```

***
* vector add
* table :table name
* content: text
* vector: vector data
```
curl -X POST http://localhost:8888/api/insert \
  -H "Content-Type: application/json" \
  -d '{"table": "document", "content": "hello", "vector": "[0.01 , 0.03, 0.04]"}'
```

* vector select
* table :table name
* limit: max record
* vector: vector data
```
curl -X POST http://localhost:8888/api/select \
  -H "Content-Type: application/json" \
  -d '{"table": "document", "limit": 5 ,"vector": "[0.02 , 0.03, 0.14]"}'
```

* vector delete
* table :table name
* id: id value

```
curl -X POST http://localhost:8888/api/delete \
  -H "Content-Type: application/json" \
  -d '{"table": "document", "id": "5d513249-d868-4688-822e-cdd3978c87eb"}'
```

***
### blog

