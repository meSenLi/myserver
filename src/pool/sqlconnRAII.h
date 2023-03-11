#ifndef __SQLCONNRALL_H__
#define __SQLCONNRALL_H__

#include "sqlconnpool.h"

class SqlConnRAII {
  public:
    SqlConnRAII(MYSQL **sql, SqlConnPool *conn_pool) {
        assert(conn_pool);
        *sql = conn_pool->GetConn();
        sql_ = *sql;
        conn_pool_ = conn_pool;
    }
    ~SqlConnRAII() {
        if (sql_) {
            conn_pool_->FreeConn(sql_);
        }
    }

  private:
    MYSQL *sql_;
    SqlConnPool *conn_pool_;
};

#endif // __SQLCONNRALL_H__