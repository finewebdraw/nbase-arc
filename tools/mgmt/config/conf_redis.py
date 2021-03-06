#
# Copyright 2015 Naver Corp.
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# Redis configuration file template

REDIS_CONFIG = (
    # <key> <value>
    ["smr-local-port", "-1"],
    ["port", "-1"],
    # nBase-ARC
    ["cronsave", [[0,3]]],

    ################################ GENERAL ######################################
    ["daemonize", "yes"],
    ["pidfile", "redis.pid"],
    ["timeout", "0"],
    ["tcp-keepalive", "60"],
    ["loglevel", "notice"],
    ["logfile", "redis.log"],
    ["databases", "1"],

    ################################ SNAPSHOTTING #################################
    # nBase-ARC default disabled
    ["save", "\"\""],
    # nBase-ARC
    ["seqsave", "500"],
    ["stop-writes-on-bgsave-error", "no"],
    ["rdbcompression", "yes"],
    ["rdbchecksum", "yes"],
    ["dbfilename", "dump.rdb"],
    # nBase-ARC the total number of rdb backups
    ["number-of-rdb-backups", "0"],
    ["dir", "./"],

    ################################# REPLICATION #################################
    ["slave-serve-stale-data", "yes"],
    ["slave-read-only", "yes"],
    ["repl-disable-tcp-nodelay", "no"],
    ["slave-priority", "100"],

    ############################## APPEND ONLY MODE ###############################
    ["appendonly", "no"],
    ["appendfilename", '"appendonly.aof"'],
    ["appendfsync", "everysec"],
    ["no-appendfsync-on-rewrite", "no"],
    ["auto-aof-rewrite-percentage", "100"],
    ["auto-aof-rewrite-min-size", "64mb"],

    ################################ LUA SCRIPTING ################################
    ["lua-time-limit", "5000"],

    ################################## SLOW LOG ###################################
    ["slowlog-log-slower-than", "10000"],
    ["slowlog-max-len", "128"],

    ############################# Event notification ##############################
    ["notify-keyspace-events", "\"\""],

    ############################### ADVANCED CONFIG ###############################
    ["hash-max-ziplist-entries", "512"],
    ["hash-max-ziplist-value", "64"],
    ["list-max-ziplist-entries", "512"],
    ["list-max-ziplist-value", "64"],
    ["set-max-intset-entries", "512"],
    ["zset-max-ziplist-entries", "128"],
    ["zset-max-ziplist-value", "64"],
    ["activerehashing", "yes"],
    ["client-output-buffer-limit", ["normal 0 0 0", "pubsub 32mb 8mb 60", "slave 256mb 64mb 60"]],
    ["hz", "10"],
    ["aof-rewrite-incremental-fsync", "yes"]
)
