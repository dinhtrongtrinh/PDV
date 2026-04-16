package cz.cvut.fel.agents.pdv.bfs;

import cz.cvut.fel.agents.pdv.dsand.Message;

public class PathMessage extends Message {
    protected String path;

    public PathMessage(String path){
        this.path = path;
    }

    public String getPath(){
        return path;
    }
}