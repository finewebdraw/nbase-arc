package com.navercorp.nbasearc.confmaster.heartbeat;

import java.util.concurrent.atomic.AtomicLong;

import com.navercorp.nbasearc.confmaster.repository.MemoryObjectMapper;

public class HBRefData {
    
    private ZKData zkData = new ZKData();
    private String lastState;
    private long lastStateTimestamp;
    private boolean submitMyOpinion;
    
    private AtomicLong refDataVersion = new AtomicLong(0L);
    
    private static final MemoryObjectMapper mapper = new MemoryObjectMapper();

    public synchronized HBRefData setZkData(String state, long stateTimestamp, int version) {
        this.zkData.state = state;
        this.zkData.stateTimestamp = stateTimestamp;
        this.zkData.version = version;
        return this;
    }
    
    public synchronized ZKData getZkData() {
        return zkData;
    }
    
    public synchronized String getLastState() {
        return lastState;
    }
    
    public synchronized HBRefData setLastState(String lastState) {
        this.lastState = lastState;
        return this;
    }
    
    public synchronized long getLastStateTimestamp() {
        return lastStateTimestamp;
    }
    
    public synchronized HBRefData setLastStateTimestamp(long lastStateTimestamp) {
        this.lastStateTimestamp = lastStateTimestamp;
        return this;
    }
    
    public synchronized boolean isSubmitMyOpinion() {
        return submitMyOpinion;
    }
    
    public synchronized HBRefData setSubmitMyOpinion(boolean submitMyOpinion) {
        this.submitMyOpinion = submitMyOpinion;
        return this;
    }
    
    public synchronized long getHbcRefDataVersion() {
        return refDataVersion.get();
    }
    
    public synchronized long increaseAndGetHbcRefDataVersion() {
        return refDataVersion.incrementAndGet();
    }
    
    @Override
    public String toString() {
        try {
            return mapper.writeValueAsString(this);
        } catch (Exception e) {
            throw new RuntimeException(e.getMessage());
        }
    }

    public class ZKData {
        public String state;
        public long stateTimestamp;
        public int version;
    }
    
}