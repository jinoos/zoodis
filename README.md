# Zoodis

`Zoo`keeper + Re`dis`

Watchdog for Redis server and tracing stats on Zookeeper with ephemeral node. 

## INSTALL

### Dependency

- Zookeeper library multi-threaded version.
- Redis : http://redis.io/download

Simple installation with git repository

    ]$ git clone https://github.com/jinoos/zoodis.git
    ]$ cd zoodis
    ]$ ./configure
    ]$ make
  
### Configure options

    ]$ ./configure
    
With specific Zookeeper installation directory as compile version on /usr/local/zookeeper

    ]$ ./configure --with-zookeeper=/usr/local/zookeeper
    
For static compile with Zookeeper library archive file.
    
    ]$ ./configure --with-zookeeper-static=/usr/lib/x86_64-linux-gnu/libzookeeper_mt.a
    
And

    ]$ ./configure --prefix=/usr/local/zoodis
    
### Run

Regarding
- Zookeeper connection hostname is `192.168.1.2:2181`
- Redis installed /path/redis and configured port 6379 on same machine

Execute Redis server with `/path/conf/redis.conf` and Zoodis will ping test via port `6379`

    ]$ ./zoodis \
          --redis-bin=/path/bin/redis-server \
          --redis-conf=/path/conf/redis.conf \
          --redis-port=6379

With Zookeeper tracing. 

    ]$ ./zoodis \
          --redis-bin=/path/bin/redis-server \
          --redis-conf=/path/conf/redis.conf \
          --redis-port=6379 \
          --zoo-host=192.168.1.2:2181 \
          --zoo-path=/Redis \
          --zoo-nodename=node-1

then, you can see Zookeeper node in /Redis/node-1 when redis server is working well.

If you want to keep Redis server alive, use `--keepalive` option as below, Zoodis keep in touch via the port and starting it up when Redis dead.

    ]$ ./zoodis \
          --redis-bin=/path/bin/redis-server \
          --redis-conf=/path/conf/redis.conf \
          --redis-port=6379 \
          --keepalive

Zoodis watchs Redis two ways, PID monitoring and [PING](http://redis.io/commands/ping) command test. When Zoodis do restart Redis when recieved SIGCHLD signal. And if reached fail count to `MAX_FAIL_COUNT`(default:2) continuously (`TODO`:--redis-max-fail-count), then kill Redis process and restart.  

### Options

Please use `--help`, and see other options.
