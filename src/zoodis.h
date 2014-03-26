#ifndef _ZOODIS_H_
#define _ZOODIS_H_

#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <zookeeper/zookeeper.h>

#include "logging.h"
#include "mstr.h"
//#include "zookeeper_util.h"

#define DEFAULT_KEEPALIVE_INTERVAL      1
#define DEFAULT_ZOO_NODEDATA            "1"
#define DEFAULT_ZOO_TIMEOUT             5000 // msec
#define DEFAULT_ZOO_CONNECT_WAIT_INTERVAL   5 // sec
#define DEFAULT_REDIS_PORT              6379
#define DEFAULT_REDIS_IP                "127.0.0.1"
#define DEFAULT_REDIS_PING_INTERVAL     5   // sec
#define DEFAULT_REDIS_PING              "PING\r\n"
#define DEFAULT_REDIS_PONG              "+PONG\r\n"
#define DEFAULT_REDIS_PONG_TIMEOUT_SEC  1
#define DEFAULT_REDIS_PONG_TIMEOUT_USEC 0
#define DEFAULT_REDIS_MAX_FAIL_COUNT    2

#define ZU_RETURN_PRINT(x)      zu_return_print(__FILE__, __LINE__, x)

enum zoo_stat
{
    ZOO_STAT_NOT_CONNECTED,
    ZOO_STAT_CONNECTIONG,
    ZOO_STAT_CONNECTED,
};

enum zoo_res
{
    ZOO_RES_OK,
    ZOO_RES_ERROR,
};

enum redis_stat
{
    // nothing to do for redis-server
    // or received SIGCHLD
    REDIS_STAT_NONE,

    // after fork process
    REDIS_STAT_EXECUTED,

    // after received OK to check ping with redis-server
    REDIS_STAT_OK,

    // when failed ping check
    REDIS_STAT_ABNORMAL,

    // after send kill signal to redis-server
    REDIS_STAT_KILLING,
};

struct zoodis
{
    int log_level;

    int keepalive;
    int keepalive_interval;

    int redis_sock;
    int redis_port;
    struct sockaddr_in redis_addr;
    struct mstr *redis_bin;
    struct mstr *redis_conf;
    enum redis_stat redis_stat;
    pid_t redis_pid;
    int redis_fail_count;
    int redis_max_fail_count;
    int redis_ping_interval;
    int redis_pong_timeout_sec;
    int redis_pong_timeout_usec;

    int zookeeper;
    int zoo_timeout;
    int zoo_connect_wait_interval;
    enum zoo_stat zoo_stat;
    struct mstr *zoo_host;
    struct mstr *zoo_path;
    struct mstr *zoo_nodepath;
    struct mstr *zoo_nodename;
    struct mstr *zoo_nodedata;

    zhandle_t           *zh;
    const clientid_t    *zid;

};

void print_version(char **argv);
void print_help(char **argv);
void set_logging(char *optarg);
int check_redis_options(struct zoodis *zoodis);
struct mstr* check_redis_bin(char *optarg);
struct mstr* check_redis_conf(char *optarg);
struct mstr* check_zoo_host(char *optarg);
struct mstr* check_zoo_path(char *optarg);
struct mstr* check_zoo_nodename(char *optarg);
struct mstr* check_zoo_nodedata(char *optarg);
int check_zoo_options(struct zoodis *zoodis);
int check_option_int(char *optarg, int def);

void zu_con_watcher(zhandle_t *zh, int type, int state, const char *path, void *data);
void zu_set_log_stream(FILE *fd);
void zu_set_log_level(int level);
void zu_return_print(const char *f, int l, int ret);
enum zoo_res zu_connect(struct zoodis *z);
enum zoo_res zu_create_ephemeral(struct zoodis *z);
enum zoo_res zu_remove_ephemeral(struct zoodis *z);

void exec_redis();
void signal_sigchld(int sig);
void signal_sigint(int sig);
void redis_health();

#endif // _ZOODIS_H_

