#ifndef __SQLCONNPOOL_H__
#define __SQLCONNPOOL_H__

#include "../log/log.h"
#include <mutex>
#include <mysql/mysql.h>
#include <queue>
#include <semaphore.h>
#include <string>
#include <thread>

// 这个类用来管理mysql的连接
class SqlConnPool {
  public:
    static SqlConnPool *Instance();
    MYSQL *GetConn();
    void FreeConn(MYSQL *conn);
    int GetFreeConnCount();
    void Init(const char *host, int port, const char *user, const char *pwd,
              const char *db_name, int conn_size);
    void ClosePool();

  private:
    SqlConnPool();
    ~SqlConnPool();
    int MAX_CONN_;
    int use_count_;
    int free_count_;
    std::queue<MYSQL *> conn_que_;
    std::mutex mtx_;
    sem_t sem_id_;
};

#endif //__SQLCONNPOOL_H__