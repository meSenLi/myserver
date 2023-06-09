#include "sqlconnpool.h"

SqlConnPool::SqlConnPool() {
    use_count_ = 0;
    free_count_ = 0;
}
SqlConnPool *SqlConnPool::Instance() {
    static SqlConnPool conn_pool;
    return &conn_pool;
}
void SqlConnPool::Init(const char *host, int port, const char *user,
                       const char *pwd, const char *db_name,
                       int conn_size = 10) {
    assert(conn_size > 0);
    for (int i = 0; i < conn_size; i++) {
        MYSQL *sql = nullptr;
        sql = mysql_init(sql);
        if (!sql) {
            LOG_ERROR("Mysql init error !");
            assert(sql);
        }
        sql =
            mysql_real_connect(sql, host, user, pwd, db_name, port, nullptr, 0);
        if (!sql) {
            LOG_ERROR("Mysql connect error!");
        }
        conn_que_.push(sql);
    }
    MAX_CONN_ = conn_size;
    sem_init(&sem_id_, 0, MAX_CONN_);
}

MYSQL *SqlConnPool::GetConn() {
    MYSQL *sql = nullptr;
    if (conn_que_.empty()) {
        LOG_WARN("sqlconnpool busy!");
        return nullptr;
    }
    sem_wait(&sem_id_);
    {
        std::lock_guard<std::mutex> lock(mtx_);
        sql = conn_que_.front();
        conn_que_.pop();
    }
    return sql;
}

void SqlConnPool::FreeConn(MYSQL *sql) {
    assert(sql);
    std::lock_guard<std::mutex> lock(mtx_);
    conn_que_.push(sql);
    sem_post(&sem_id_);
}

void SqlConnPool::ClosePool() {
    std::lock_guard<std::mutex> lock(mtx_);
    while (!conn_que_.empty()) {
        auto item = conn_que_.front();
        conn_que_.pop();
        mysql_close(item);
    }
    mysql_library_end();
}

int SqlConnPool::GetFreeConnCount() {
    std::lock_guard<std::mutex> lock(mtx_);
    return conn_que_.size();
}
SqlConnPool::~SqlConnPool() { ClosePool(); }