package cz.cvut.fel.agents.pdv.bfs;

import cz.cvut.fel.agents.pdv.dsand.Simulation;

public class FailingNode2 {
    public static void main(String[] args) {
        Simulation sim = new Simulation(new DSConfig(1, 0.5));
        sim.run();
    }
}
