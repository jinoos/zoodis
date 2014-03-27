#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "zoodis.h"
#include "version.h"
#include "utime.h"

static struct zoodis zoodis;

int main(int argc, char *argv[])
{
    signal(SIGCHLD, signal_sigchld);
    signal(SIGINT, signal_sigint);
    signal(SIGPIPE, signal_sigint);
    signal(SIGTERM, signal_sigint);
    signal(SIGTSTP, signal_sigint);
    signal(SIGHUP, signal_sigint);

    log_level(_LOG_DEBUG);
    log_msg("Start zoodis.");

    zoodis.keepalive_interval           = DEFAULT_KEEPALIVE_INTERVAL;

    zoodis.zoo_stat                     = ZOO_STAT_NOT_CONNECTED;
    zoodis.zoo_timeout                  = DEFAULT_ZOO_TIMEOUT;
    zoodis.zoo_connect_wait_interval    = DEFAULT_ZOO_CONNECT_WAIT_INTERVAL;

    zoodis.redis_port                   = DEFAULT_REDIS_PORT;
    zoodis.redis_ping_interval          = DEFAULT_REDIS_PING_INTERVAL;
    zoodis.redis_pong_timeout_sec       = DEFAULT_REDIS_PONG_TIMEOUT_SEC;
    zoodis.redis_pong_timeout_usec      = DEFAULT_REDIS_PONG_TIMEOUT_USEC;
    zoodis.redis_max_fail_count         = DEFAULT_REDIS_MAX_FAIL_COUNT;
    zoodis.pid_file                     = NULL;

    zoodis.redis_stat = REDIS_STAT_NONE;

    static struct option long_options[] =
    {
        {"help",                no_argument,        0,  'h'},
        {"version",             no_argument,        0,  'v'},
        {"log-level",           required_argument,  0,  'l'},
        {"pid-file",            required_argument,  0,  'f'},
        {"keepalive",           no_argument,        0,  'k'},
        {"keepalive-interval",  required_argument,  0,  'i'},
        {"redis-bin",           required_argument,  0,  'b'},
        {"redis-conf",          required_argument,  0,  'c'},
        {"redis-port",          required_argument,  0,  'r'},
        {"redis-ping-interval", required_argument,  0,  's'},
        {"zoo-host",            required_argument,  0,  'z'},
        {"zoo-path",            required_argument,  0,  'p'},
        {"zoo-nodename",        required_argument,  0,  'n'},
        {"zoo-nodedata",        required_argument,  0,  'd'},
        {"zoo-timeout",         required_argument,  0,  't'},
        {0, 0, 0, 0}
    };

    enum zoo_res zres;

    if(argc < 3)
        print_help(argv);

    while (1)
    {
        /* getopt_long stores the option index here. */
        int option_index = 0;
        int opt;

        opt = getopt_long_only(argc, argv, "", long_options, &option_index);

        /* Detect the end of the options. */
        if (opt == -1)
            break;

        switch (opt)
        {
            case 'h':
                print_help(argv);
                break;

            case 'v':
                print_version(argv);
                break;

            case 'l':
                set_logging(optarg);
                break;

            case 'k':
                zoodis.keepalive = 1;
                break;

            case 'f':
                zoodis.pid_file = check_pid_file(optarg);

            case 'i':
                zoodis.keepalive_interval = check_option_int(optarg, DEFAULT_KEEPALIVE_INTERVAL);
                break;

            case 'b':
                zoodis.redis_bin = check_redis_bin(optarg);
                break;

            case 'c':
                zoodis.redis_conf = check_redis_conf(optarg);
                break;

            case 'r':
                zoodis.redis_port = check_option_int(optarg, DEFAULT_REDIS_PORT);
                break;

            case 's':
                zoodis.redis_ping_interval = check_option_int(optarg, DEFAULT_REDIS_PING_INTERVAL);
                break;

            case 'z':
                zoodis.zoo_host = check_zoo_host(optarg);
                break;

            case 'p':
                zoodis.zoo_path = check_zoo_path(optarg);
                break;

            case 'n':
                zoodis.zoo_nodename = check_zoo_nodename(optarg);
                break;

            case 'd':
                zoodis.zoo_nodedata = check_zoo_nodedata(optarg);
                break;

            case 't':
                zoodis.zoo_timeout = check_option_int(optarg, DEFAULT_ZOO_TIMEOUT);
                break;

            default:
                exit_proc(-1);
        }
    }

    zoodis.zookeeper = check_zoo_options(&zoodis);

    if(zoodis.zoo_nodedata == NULL)
        zoodis.zoo_nodedata = mstr_alloc_dup(DEFAULT_ZOO_NODEDATA, strlen(DEFAULT_ZOO_NODEDATA));

    check_redis_options(&zoodis);

    if(zoodis.zookeeper)
    {
        zoodis.zoo_nodepath = mstr_concat(3, zoodis.zoo_path->data, "/", zoodis.zoo_nodename->data);
        zu_set_log_level(log_level(0));
        zu_set_log_stream(log_fd(stdout));
        zres = zu_connect(&zoodis);
        if(zres != ZOO_RES_OK)
        {
            exit_proc(-1);
        }
    }

//    sleep(1);
//    while(zoodis.zoo_stat != ZOO_STAT_CONNECTED)
//    if(zoodis.zoo_stat != ZOO_STAT_CONNECTED)
//    {
//        log_warn("Zookeeper: is not connected yet.");
//        sleep(zoodis.zoo_connect_wait_interval);
//    }

    exec_redis();
    redis_health();
    return 0;
}

void zu_con_watcher(zhandle_t *zh, int type, int state, const char *path, void *data)
{
    struct zoodis *z = (struct zoodis*) data;
    log_info("Zookeeper: Connection status changed. type:%d state:%d", type, state);

    if(state == ZOO_CONNECTED_STATE)
    {
        log_info("Zookeeper: connected.");
        z->zoo_stat = ZOO_STAT_CONNECTED;
    }
}

enum zoo_res zu_connect(struct zoodis *z)
{
    zhandle_t *zh = zookeeper_init(z->zoo_host->data, zu_con_watcher, z->zoo_timeout, z->zid, z, 0);
    zoodis.zoo_stat = ZOO_STAT_CONNECTIONG;

    log_info("Zookeeper: Trying to connect to zookeeper %s", z->zoo_host->data);

    if(!zh)
    {
        log_err("Zookeeper: Cannot initialize zhandle.");
        log_err("Zookeeper: -- %s", strerror(errno));
        zoodis.zoo_stat = ZOO_STAT_NOT_CONNECTED;
        return ZOO_RES_ERROR;
    }

    z->zh = zh;
    z->zid = zoo_client_id(zh);

    return ZOO_RES_OK;
}


void set_logging(char *optarg)
{
    if(optarg == NULL)
        log_level(_LOG_DEFAULT);

    else if(strncmp(_LOG_SSDEBUG, optarg, strlen(_LOG_SSDEBUG)) == 0)
        log_level(_LOG_DEBUG);

    else if(strncmp(_LOG_SSINFO, optarg, strlen(_LOG_SSINFO)) == 0)
        log_level(_LOG_INFO);

    else if(strncmp(_LOG_SSWARN, optarg, strlen(_LOG_SSWARN)) == 0)
        log_level(_LOG_WARN);

    else if(strncmp(_LOG_SSERR, optarg, strlen(_LOG_SSERR)) == 0)
        log_level(_LOG_ERR);

    else if(strncmp(_LOG_SSINFO, optarg, strlen(_LOG_SSINFO)) == 0)
        log_level(_LOG_INFO);
    else 
        log_level(_LOG_DEFAULT);

//    zu_set_log_level(log_level(0));
}

void zu_set_log_stream(FILE *fd)
{
    zoo_set_log_stream(fd);
}

void zu_set_log_level(int level)
{

    switch(level)
    {
    case _LOG_DEBUG:
        zoo_set_debug_level(ZOO_LOG_LEVEL_DEBUG);
        break;
    case _LOG_INFO:
        zoo_set_debug_level(ZOO_LOG_LEVEL_INFO);
        break;
    case _LOG_WARN:
        zoo_set_debug_level(ZOO_LOG_LEVEL_WARN);
        break;
    case _LOG_ERR:
        zoo_set_debug_level(ZOO_LOG_LEVEL_ERROR);
        break;
    default:
        zoo_set_debug_level(ZOO_LOG_LEVEL_INFO);
        break;
    }
}

void zu_return_print(const char *f, int l, int ret)
{
    switch(ret)
    {
    case ZOK:
        log_info("%s(%d) Zookeeper: OK.", f, l);
        break;

    case ZSYSTEMERROR:
        log_err("%s(%d) Zookeeper: System error.", f, l);
        break;

    case ZRUNTIMEINCONSISTENCY:
        log_err("%s(%d) Zookeeper: A runtime inconsistency was found.", f, l);
        break;

    case ZDATAINCONSISTENCY:
        log_err("%s(%d) Zookeeper: A data inconsistency was found.", f, l);
        break;

    case ZCONNECTIONLOSS:
        log_err("%s(%d) Zookeeper: Connection to the server has been lost.", f, l);
        break;

    case ZMARSHALLINGERROR:
        log_err("%s(%d) Zookeeper: Error while marshalling or unmarshalling data.", f, l);
        break;

    case ZUNIMPLEMENTED:
        log_err("%s(%d) Zookeeper: Operation is unimplemented.", f, l);
        break;

    case ZOPERATIONTIMEOUT:
        log_err("%s(%d) Zookeeper: Operation timeout.", f, l);
        break;

    case ZBADARGUMENTS:
        log_err("%s(%d) Zookeeper: Invalid arguments.", f, l);
        break;

    case ZINVALIDSTATE:
        log_err("%s(%d) Zookeeper: Invliad zhandle state.", f, l);
        break;

    case ZAPIERROR:
        log_err("%s(%d) Zookeeper: API errors.", f, l);
        break;

    case ZNONODE:
        log_err("%s(%d) Zookeeper: Node does not exist.", f, l);
        break;

    case ZNOAUTH:
        log_err("%s(%d) Zookeeper: Not authenticated.", f, l);
        break;

    case ZBADVERSION:
        log_err("%s(%d) Zookeeper: Version conflict.", f, l);
        break;

    case ZNOCHILDRENFOREPHEMERALS:
        log_err("%s(%d) Zookeeper: Ephemeral nodes may not have children.", f, l);
        break;

    case ZNODEEXISTS:
        log_err("%s(%d) Zookeeper: The node already exists.", f, l);
        break;

    case ZNOTEMPTY:
        log_err("%s(%d) Zookeeper: The node has children.", f, l);
        break;

    case ZSESSIONEXPIRED:
        log_err("%s(%d) Zookeeper: The session has been expired by the server.", f, l);
        break;

    case ZINVALIDCALLBACK:
        log_err("%s(%d) Zookeeper: Invalid callback specified.", f, l);
        break;

    case ZINVALIDACL:
        log_err("%s(%d) Zookeeper: Invalid ACL specifie.", f, l);
        break;

    case ZAUTHFAILED:
        log_err("%s(%d) Zookeeper: Client authentication faile.", f, l);
        break;

    case ZCLOSING:
        log_err("%s(%d) Zookeeper: ZooKeeper is closin.", f, l);
        break;

    case ZNOTHING:
        log_err("%s(%d) Zookeeper: (not error) no server responses to process.", f, l);
        break;

    case ZSESSIONMOVED:
        log_err("%s(%d) Zookeeper: Session moved to another server, so operation is ignored.", f, l);
        break;

    default:
        log_err("%s(%d) Zookeeper: Unknown error(%d)", f, l, ret);
    }
}

enum zoo_res zu_ephemeral_update(struct zoodis *z)
{
    if(!zoodis.zookeeper)
        return ZOO_RES_OK;

    if(zoodis.zoo_stat != ZOO_STAT_CONNECTED)
    {
        log_err("Zookeeper: not connected yet. STAT:%d", zoodis.zoo_stat);
        return ZOO_RES_ERROR;
    }


    if(zoodis.redis_stat == REDIS_STAT_OK)
        return zu_create_ephemeral(z);
    else
        return zu_remove_ephemeral(z);
}


enum zoo_res zu_create_ephemeral(struct zoodis *z)
{
    int res;
    int bufsize = 512;
    char buffer[bufsize]; // no reason, added due to some of example, need to be done.
    int buffer_len = bufsize;
    memset(buffer, 0x00, bufsize);

    res = zoo_get(z->zh, z->zoo_nodepath->data, 0, buffer, &buffer_len, 0);

    if(res == ZOK)
    {
        if(buffer_len == z->zoo_nodedata->len && strncmp(z->zoo_nodedata->data, buffer, z->zoo_nodedata->len) == 0)
        {
            return ZOO_RES_OK;
        }else
        {
            zu_remove_ephemeral(z);
        }
    }else if(res != ZOK && res != ZNONODE)
    {
        ZU_RETURN_PRINT(res);
        if(res == ZINVALIDSTATE)
        {
            zookeeper_close(z->zh);
            zoodis.zoo_stat = ZOO_STAT_NOT_CONNECTED;
            z->zid = NULL;
            log_warn("Zookeeper: error, trying to re-connect.");
            zu_connect(z);
        }
        // exit_proc(-1);
    }

    res = zoo_create(z->zh, z->zoo_nodepath->data, z->zoo_nodedata->data, strlen(z->zoo_nodedata->data), &ZOO_READ_ACL_UNSAFE, ZOO_EPHEMERAL, buffer, sizeof(buffer)-1);
    if(res != ZOK)
    {
        ZU_RETURN_PRINT(res);
        if(res == ZINVALIDSTATE)
        {
            zookeeper_close(z->zh);
            zoodis.zoo_stat = ZOO_STAT_NOT_CONNECTED;
            z->zid = NULL;
            log_warn("Zookeeper: error, trying to re-connect.");
            zu_connect(z);
        }
    }

    return ZOO_RES_OK;
}

enum zoo_res zu_remove_ephemeral(struct zoodis *z)
{
    int res;

    res =  zoo_delete(z->zh, z->zoo_nodepath->data, -1);
    
    if(res != ZOK && res != ZNONODE)
    {
        ZU_RETURN_PRINT(res);
        if(res == ZINVALIDSTATE)
        {
            zookeeper_close(z->zh);
            zoodis.zoo_stat = ZOO_STAT_NOT_CONNECTED;
            z->zid = NULL;
            log_warn("Zookeeper: error, trying to re-connect.");
            zu_connect(z);
        }
        //exit_proc(-1);
    }

    return ZOO_RES_OK;
}

struct mstr* check_redis_bin(char *optarg)
{
    struct stat stat_bin;
    int res;

    if(strlen(optarg) == 0)
    {
        log_err("path of redis-bin option must not be empty.");
        exit_proc(-1);
    }

    res = lstat(optarg, &stat_bin);

    // check stat readable
    if(res < 0)
    {
        log_err("%s, %s\n", strerror(errno), optarg);
        exit_proc(-1);
    }

    // check regular file
    if(!S_ISREG(stat_bin.st_mode))
    {
        log_err("%s is not a regular file.", optarg);
        exit_proc(-1);
    }

    // check executable or not
    if( stat_bin.st_mode & S_IXOTH ||
        (getgid() == stat_bin.st_gid && stat_bin.st_mode & S_IXGRP) ||
        (getuid() == stat_bin.st_uid && stat_bin.st_mode & S_IXUSR ))
    {
        return mstr_alloc_dup(optarg, strlen(optarg));
    }else
    {
        log_err("%s is not an executable.", optarg);
        exit_proc(-1);
        return NULL;
    }
}

struct mstr* check_redis_conf(char *optarg)
{
    struct stat stat_bin;
    int res;

    if(strlen(optarg) == 0)
    {
        log_err("path of redis-conf option must not be empty.");
        exit_proc(-1);
    }

    res = lstat(optarg, &stat_bin);

    // check stat readable
    if(res < 0)
    {
        log_err("%s, %s", strerror(errno), optarg);
        exit_proc(-1);
    }

    // check regular file
    if(!S_ISREG(stat_bin.st_mode))
    {
        log_err("%s is not a regular file.", optarg);
        exit_proc(-1);
    }

    // check executable or not
    if( stat_bin.st_mode & S_IROTH ||
        (getgid() == stat_bin.st_gid && stat_bin.st_mode & S_IRGRP) ||
        (getuid() == stat_bin.st_uid && stat_bin.st_mode & S_IRUSR ))
    {
        return mstr_alloc_dup(optarg, strlen(optarg));
    }else
    {
        log_err("%s is not an readable.", optarg);
        exit_proc(-1);
        return NULL;
    }
}

int check_redis_port(char *optarg)
{
    int i = atoi(optarg);
    if(i > 0)
    {
        return i;
    }else
    {
        return DEFAULT_REDIS_PORT;
    }
}

int check_option_int(char *optarg, int def)
{
    int i = atoi(optarg);
    if(i > 0)
        return i;
    else
        return def;
}

int check_keepalive_interval(char *optarg)
{
    int i = atoi(optarg);
    if(i > 0)
        return i;
    else
        return DEFAULT_KEEPALIVE_INTERVAL;
}

struct mstr* check_zoo_host(char *optarg)
{
    if(optarg == NULL || strlen(optarg) == 0)
        return NULL;
    return mstr_alloc_dup(optarg, strlen(optarg));
}

struct mstr* check_zoo_path(char *optarg)
{
    if(optarg == NULL || strlen(optarg) == 0)
        return NULL;
    return mstr_alloc_dup(optarg, strlen(optarg));
}

struct mstr* check_zoo_nodename(char *optarg)
{
    if(optarg == NULL || strlen(optarg) == 0)
        return NULL;
    return mstr_alloc_dup(optarg, strlen(optarg));
}

struct mstr* check_zoo_nodedata(char *optarg)
{
    if(optarg == NULL || strlen(optarg) == 0)
        return NULL;
    
    return mstr_alloc_dup(optarg, strlen(optarg));
}

int check_redis_options(struct zoodis *zoodis)
{
    if(zoodis->redis_bin == NULL || zoodis->redis_conf == NULL)
    {
        log_err("--redis-bin, --redis-conf are mandatory.");
        exit_proc(-1);
    }

    if(zoodis->redis_port == 0)
        zoodis->redis_port = DEFAULT_REDIS_PORT;

    memset(&zoodis->redis_addr, 0, sizeof(struct sockaddr_in));

    zoodis->redis_addr.sin_family = AF_INET;
    zoodis->redis_addr.sin_port = htons(zoodis->redis_port);
    zoodis->redis_addr.sin_addr.s_addr= inet_addr(DEFAULT_REDIS_IP);

    return 1;
}

const char* check_pid_file(const char *pid_file)
{
    int fd, res;
    pid_t pid = getpid();
    struct flock flock;


    zoodis.pid_fp = fopen(pid_file, "r+");
    if(zoodis.pid_fp == NULL)
    {
        zoodis.pid_fp = fopen(pid_file, "w");
        if(zoodis.pid_fp == NULL)
        {
            log_err("Cannot open pid file %s, %s", pid_file, strerror(errno));
            exit_proc(-1);
        }
    }

    fd = fileno(zoodis.pid_fp);

    memset(&flock, 0x00, sizeof(struct flock));
    flock.l_type = F_WRLCK;

    res = fcntl(fd, F_SETLK, &flock);
    if(res < 0)
    {
        log_err("Another zoodis process is running. PID:%s", pid_file);
        fclose(zoodis.pid_fp);
        exit_proc(-1);
    }

    log_info("PID file %s", pid_file);

    fseek(zoodis.pid_fp, 0L, SEEK_SET);
    res = fprintf(zoodis.pid_fp, "%d", pid);
    res = ftruncate(fd, (off_t)ftell(zoodis.pid_fp));
    fflush(zoodis.pid_fp);
    return pid_file;
}

int check_zoo_options(struct zoodis *zoodis)
{
    if(zoodis->zoo_host != NULL || zoodis->zoo_path != NULL || zoodis->zoo_nodename != NULL)
    {
        if(zoodis->zoo_host == NULL || zoodis->zoo_path == NULL || zoodis->zoo_nodename == NULL)
        {
            log_err("--zoo-host, --zoo-path and --zoo-nodename are mandatory for using zookeeper server.");
            exit_proc(-1);
        }
    }

    if(zoodis->zoo_nodedata != NULL)
    {
        if(zoodis->zoo_host == NULL || zoodis->zoo_path == NULL || zoodis->zoo_nodename == NULL)
        {
            log_err("--zoo-host, --zoo-path and --zoo-nodename are mandatory for using zookeeper server.");
            exit_proc(-1);
        }
    }

    if(zoodis->zoo_host == NULL || zoodis->zoo_path == NULL || zoodis->zoo_nodename == NULL)
    {
        return 0;
    }
    
    return 1;
}

void print_version(char **argv)
{
    printf("Zoodis %d.%d.%d\n", _VER_MAJOR, _VER_MINOR, _VER_PATCH);
    exit(0);
}


void print_help(char **argv)
{
    printf("\n");
    printf("Usage: %s [Options] --redis-bin=REDIS_SERVER_PATH --redis-conf=REDIS_CONF_PATH --redis-port=PORT\n", argv[0]);
    printf("\n");
    printf("    --redis-bin=PATH\n");
    printf("                    Path of redis-server daemon.\n");
    printf("    --redis-conf=PATH\n");
    printf("                    Configure path of redis.\n");
    printf("    --redis-port=PORT\n");
    printf("                    Configure port of redis.\n");
    printf("                    It's going to be used to health check.\n");
    printf("Options\n");
    printf("    --keepalive\n");
    printf("                    Keep continue to restart redis-server when it's down.\n");
    printf("    --keepalive-interval=SECOND\n");
    printf("                    Restart interval time since catch down signal.\n");
    printf("                    If not set this, default is 1 second.\n");
    printf("                    This option works with keepalive option.\n");
    printf("    --redis-ping-interval=SECONDS\n");
    printf("                    Interval seconds while ping(health) check.\n");
    printf("    --zoo-host=ZOOKEEPERHOSTS\n");
    printf("                    Connection string for zookeeper server.\n");
    printf("    --zoo-path=NODEPATH\n");
    printf("                    Parent zookeeper path.\n");
    printf("                    This option works with zoo-host option.\n");
    printf("    --zoo-nodename=NODENAME\n");
    printf("                    Create node name what is guarrentee redis-server alive\n");
    printf("                    This option works with zoo-host and zoo-path option.\n");
    printf("    --zoo-nodedata=NODEDATA\n");
    printf("                    What data string in the zoo-nodename node.\n");
    printf("                    Default is \"1\"\n");
    printf("                    This option works with zoo-host and zoo-path option.\n");
    printf("    --pid-file=PATH\n");
    printf("                    Pid file path.\n");
    printf("    --log-level=[DEBUG|INFO|WARN|ERROR]\n");
    printf("                    Set logging level.\n");
    printf("                    Default is WARN\n");
    printf("    --version\n");
    printf("                    Print version.\n");
    printf("    --help\n");
    printf("                    This message.\n");
    exit(0);
}

void exec_redis()
{
    pid_t pid;

    pid = fork();

    if(pid < 0)
    {
        log_err("Redis: Failed forming process, %s", strerror(errno));
        exit_proc(-1);
    }

    if(pid == 0)
    {
        if(execl(zoodis.redis_bin->data, zoodis.redis_bin->data, zoodis.redis_conf->data, NULL) < 0)
        {
            log_err("Redis: failed to execute redis daemon. %s", strerror(errno));
        }
    }else
    {
        zoodis.redis_pid = pid;
        zoodis.redis_stat = REDIS_STAT_EXECUTED;
        log_info("Redis: started redis daemon.");
        sleep(DEFAULT_REDIS_SLEEP_AFTER_EXEC);
        redis_health();
    }

    return;
}

void redis_kill()
{
    if(zoodis.redis_pid != 0)
    {
        log_info("Redis: killing daemon. PID:%d", zoodis.redis_pid);
        kill(zoodis.redis_pid, SIGTERM);
        zoodis.redis_stat = REDIS_STAT_KILLING;
        int stat, pid;
        pid = waitpid(zoodis.redis_pid, &stat, WNOHANG);
        zoodis.redis_stat = REDIS_STAT_NONE;
        zu_ephemeral_update(&zoodis);
        log_info("Redis: down.");
        zoodis.redis_pid = 0;
        signal(SIGCHLD, signal_sigchld);
    }
}

void signal_sigint(int sig)
{
    signal(SIGCHLD, SIG_IGN);
    log_debug("Signal: Received shutdown singal, NO:%d", sig);
    log_info("Suspending zoodis,", sig);
    zoodis.keepalive = 0;
    if(zoodis.redis_stat == REDIS_STAT_EXECUTED ||
            zoodis.redis_stat == REDIS_STAT_OK ||
            zoodis.redis_stat == REDIS_STAT_ABNORMAL)
    {
        redis_kill();
    }
    exit_proc(0);
}

void signal_sigchld(int sig)
{
    int stat, pid;
    pid = wait(&stat);
    log_debug("Signal: received SIGCHLD PID:%d, REDIS_PID:%d", pid, zoodis.redis_pid);
    if(zoodis.redis_pid == pid)
    {
        close(zoodis.redis_sock);
        zoodis.redis_sock = 0;
        zoodis.redis_stat = REDIS_STAT_NONE;
        zu_ephemeral_update(&zoodis);

        log_err("Redis: daemon has been down. Please check redis log file.");

        if(!zoodis.keepalive)
        {
            exit(-1);
        }

        sleep(zoodis.keepalive_interval);
        exec_redis();
    }
}

int redis_health_check()
{
    int res;
    char buf[1024];
    fd_set rfdset;
    struct timeval tval;

    utime_t stime, etime;
    if(!zoodis.redis_sock)
    {
        res = socket(PF_INET,SOCK_STREAM, 0);
        if(res < 0)
        {
            log_warn("Redis: cannot open socket for check redis server. %s", strerror(errno));
            return 0;
        }

        zoodis.redis_sock = res;

        res = connect(zoodis.redis_sock, (struct sockaddr*)&zoodis.redis_addr, sizeof(struct sockaddr_in));
        if(res < 0)
        {
            log_warn("Redis: cannot connect redis server %s:%d, %s", DEFAULT_REDIS_IP, zoodis.redis_port, strerror(errno));
            close(zoodis.redis_sock);
            zoodis.redis_sock = 0;
            return 0;
        }
    }

    stime = utime_time();
    res = write(zoodis.redis_sock, DEFAULT_REDIS_PING, strlen(DEFAULT_REDIS_PING));
    if(res < 0)
    {
        log_warn("Redis: error while write(send) ping to redis server. %s", strerror(errno));
        close(zoodis.redis_sock);
        zoodis.redis_sock = 0;
        return 0;
    }

    memset(buf, 0x00, 1024);

    FD_ZERO(&rfdset);
    FD_SET(zoodis.redis_sock, &rfdset);

    tval.tv_sec = zoodis.redis_pong_timeout_sec;
    tval.tv_usec = zoodis.redis_pong_timeout_usec;;

    res = select(zoodis.redis_sock+1, &rfdset, NULL, NULL, &tval);
    if(res < 0)
    {
        log_warn("Redis: redis could not response in time (timeout:%d.%d).", tval.tv_sec, tval.tv_usec);
        close(zoodis.redis_sock);
        zoodis.redis_sock = 0;
        return 0;
    }else
    {
        res = read(zoodis.redis_sock, buf, 1024);
        etime = utime_time();
        if(res < 0)
        {
            log_warn("Redis: test failed, %s", strerror(errno));
            close(zoodis.redis_sock);
            zoodis.redis_sock = 0;
            return 0;
        }else if(res == 0)
        {
            log_warn("Redis: test failed, connection closed.");
            close(zoodis.redis_sock);
            zoodis.redis_sock = 0;
            return 0;
        }else
        {
            if(strncmp(buf, DEFAULT_REDIS_PONG, 5) != 0)
            {
                log_warn("Redis: test failed, responsed not PONG, %.*s", res, buf);
                return 0;
            }else
            {
                utime_t elapsedTime = etime - stime;
                log_info("Redis: test successed. Elapsed %"PRIu64" usec", elapsedTime);
                return 1;
            }
        }
    }
}

void redis_health()
{
    int res;
    while(1)
    {
        /*
        if(zoodis.zookeeper && zoodis.zoo_stat != ZOO_STAT_CONNECTED)
        {
            continue;
        }
        */

        if(zoodis.redis_stat != REDIS_STAT_EXECUTED &&
                zoodis.redis_stat != REDIS_STAT_OK &&
                zoodis.redis_stat != REDIS_STAT_ABNORMAL)
        {
            sleep(zoodis.redis_ping_interval);
            continue;
        }

        res = redis_health_check();

        if(!res)
        {
            // failed
            zoodis.redis_fail_count++;
            if(zoodis.redis_fail_count >= zoodis.redis_max_fail_count)
            {
                zoodis.redis_fail_count = 0;
                zoodis.redis_stat = REDIS_STAT_ABNORMAL;
                redis_kill();
                zu_ephemeral_update(&zoodis);
                exec_redis();
            }
        }else
        {
            // success
            zoodis.redis_fail_count = 0;
            zoodis.redis_stat = REDIS_STAT_OK;
            zu_ephemeral_update(&zoodis);
        }
        sleep(zoodis.redis_ping_interval);
    }
}

void exit_proc(int code)
{
    if(code == 0)
    {
        log_msg("Bye.");
    }
    exit(code);
}
