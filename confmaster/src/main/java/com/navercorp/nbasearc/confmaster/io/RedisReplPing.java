package com.navercorp.nbasearc.confmaster.io;

import static com.navercorp.nbasearc.confmaster.Constant.PGS_PING;
import static com.navercorp.nbasearc.confmaster.Constant.REDIS_PONG;

import java.io.IOException;
import java.util.concurrent.Callable;

import com.navercorp.nbasearc.confmaster.config.Config;
import com.navercorp.nbasearc.confmaster.logger.Logger;
import com.navercorp.nbasearc.confmaster.server.cluster.PartitionGroupServer;
import com.navercorp.nbasearc.confmaster.server.cluster.RedisServer;

public class RedisReplPing implements Callable<RedisReplPing.Result> {
    
    private String clusterName;
    private PartitionGroupServer pgs;
    private RedisServer rs;
    private Config config;

    public RedisReplPing(String clusterName, PartitionGroupServer pgs,
            RedisServer rs, Config config) {
        this.clusterName = clusterName;
        this.pgs = pgs;
        this.rs = rs;
        this.config = config;
    }
    
    @Override
    public RedisReplPing.Result call() {
        if (pgs == null) {
            Logger.error(
                "Send smr command fail. smr variable is null. cluster: {}",
                clusterName);
            return new Result(pgs, false);
        }

        try {
            String reply = rs.executeQuery(PGS_PING);
            Logger.info("Send replicated ping to redis. {}, reply: \"{}\"", rs, reply);
            if (!reply.equals(REDIS_PONG)) {
                Logger.error("Send redis replicated ping fail. {} {}:{}, reply: \"{}\"", 
                        new Object[]{pgs, pgs.getIP(),
                                pgs.getData().getRedisPort(), reply});
                return new Result(pgs, false);
            }
        } catch (IOException e) {
            Logger.error("Send redis replicated ping fail. {} {}:{}", 
                    new Object[]{pgs, pgs.getIP(),
                            pgs.getData().getRedisPort()});
            return new Result(pgs, false);
        }

        return new Result(pgs, true);
    }
    
    public class Result {
        private final PartitionGroupServer pgs;
        private final boolean success;
        
        public Result(PartitionGroupServer pgs, boolean success) {
            this.pgs = pgs;
            this.success = success;
        }

        public PartitionGroupServer getPgs() {
            return pgs;
        }

        public boolean isSuccess() {
            return success;
        }
    }

}
