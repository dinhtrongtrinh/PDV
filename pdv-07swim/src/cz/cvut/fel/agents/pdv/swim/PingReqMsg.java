package cz.cvut.fel.agents.pdv.swim;

import cz.cvut.fel.agents.pdv.dsand.Message;

public class PingReqMsg extends Message {
    private final String targetProcessId; // ID procesu, který se má prověřit

    public PingReqMsg(String targetProcessId) {
        this.targetProcessId = targetProcessId;
    }

    public String getTargetProcessId() {
        return targetProcessId;
    }
}