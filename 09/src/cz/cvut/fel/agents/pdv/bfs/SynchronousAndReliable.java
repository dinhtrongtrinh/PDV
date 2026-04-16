package cz.cvut.fel.agents.pdv.bfs;

import cz.cvut.fel.agents.pdv.dsand.Simulation;

public class SynchronousAndReliable {
    public static void main(String[] args) {
        Simulation sim = new Simulation(new DSConfig(1, 1.0));
        sim.run();
    }
}
