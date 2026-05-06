package cz.cvut.fel.agents.pdv.swim;

import cz.cvut.fel.agents.pdv.dsand.Message;

public class AckMsg extends Message {
    private final String processId; // ID procesu, který žije

    public AckMsg(String processId) {
        this.processId = processId;
    }

    public String getProcessId() {
        return processId;
    }
}
