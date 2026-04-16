package cz.cvut.fel.agents.pdv.demo;

import cz.cvut.fel.agents.pdv.dsand.Message;

public class DummyMessage extends Message {

    long value;

    public DummyMessage(long value) {
        this.value = value;
    }

    @Override
    public String toString() {
        return "DummyMessage{" +
                "value=" + value +
                ", sender='" + sender + '\'' +
                ", recipient='" + recipient + '\'' +
                '}';
    }
}
