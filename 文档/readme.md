## 项目目标 
KV_store 

## 整体架构 
client-parser-command-execute-kvstore

## 支持的命令 
SET,GET,DEL,EXPIRE,

协议格式 
如何构建和运行
AOF 工作流程 append,rplay

## 当前限制
命令执行为单线程
KVStore 为内存 unordered_map
AOF 不保证崩溃一致性
没有事务
没有并发命令执行
未来可能改进