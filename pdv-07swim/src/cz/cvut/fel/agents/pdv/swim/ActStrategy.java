package cz.cvut.fel.agents.pdv.swim;

import cz.cvut.fel.agents.pdv.dsand.Message;
import cz.cvut.fel.agents.pdv.dsand.Pair;

import java.util.*;
import java.util.stream.Collectors;

public class ActStrategy {

    private final int maxDelay;
    private String myNodeId;
    private int roundRobinIdx;
    private final List<String> peers;

    private final Map<String, Boolean> indirectPingSent;
    private final Map<String, Set<String>> indirectWaiters;
    private final Map<String, Integer> pendingPings;

    private final Random rnd;
    private int logicalTime;
    private final int pingInterval = 5;

    public ActStrategy(int maxDelayForMessages, List<String> otherProcesses,
                       int timeToDetectKilledProcess, int upperBoundOnMessages) {

        this.maxDelay = maxDelayForMessages;
        this.rnd = new Random();

        // Zamíchání a výběr maximálně 25 náhodných sousedů
        List<String> shuffled = new ArrayList<>(otherProcesses);
        Collections.shuffle(shuffled, this.rnd);
        this.peers = shuffled.stream().limit(25).collect(Collectors.toList());

        this.roundRobinIdx = this.rnd.nextInt(this.peers.size());

        this.indirectPingSent = new HashMap<>();
        this.indirectWaiters = new HashMap<>();
        this.pendingPings = new HashMap<>();
        this.myNodeId = null;
    }

    public List<Pair<String, Message>> act(Queue<Message> inbox, String disseminationProcess) {
        List<Pair<String, Message>> outbox = new ArrayList<>();

        // === FÁZE 1: ZPRACOVÁNÍ PŘÍCHOZÍCH ZPRÁV ===
        Message incoming;
        while ((incoming = inbox.poll()) != null) {

            if (this.myNodeId == null) {
                this.myNodeId = incoming.recipient;
            }

            if (incoming instanceof PingMsg) {
                outbox.add(new Pair<>(incoming.sender, new AckMsg(null)));

                forwardAcks(incoming.sender, outbox);
                this.pendingPings.remove(incoming.sender);
                this.indirectPingSent.remove(incoming.sender);

            } else if (incoming instanceof PingReqMsg) {
                PingReqMsg req = (PingReqMsg) incoming;
                String target = req.getTargetProcessId();

                this.indirectWaiters.computeIfAbsent(target, k -> new HashSet<>()).add(incoming.sender);
                outbox.add(new Pair<>(target, new PingMsg()));

            } else if (incoming instanceof AckMsg) {
                AckMsg ack = (AckMsg) incoming;
                String aliveId = ack.getProcessId() == null ? incoming.sender : ack.getProcessId();

                forwardAcks(aliveId, outbox);
                this.pendingPings.remove(aliveId);
                this.indirectPingSent.remove(aliveId);
            }
        }

        // === FÁZE 2: ODESLÁNÍ NOVÝCH PINGŮ ===
        if (this.logicalTime % this.pingInterval == 1) {
            String targetNode;

            do {
                this.roundRobinIdx = (this.roundRobinIdx + 1) % this.peers.size();
                targetNode = this.peers.get(this.roundRobinIdx);
            } while (this.indirectPingSent.containsKey(targetNode));

            outbox.add(new Pair<>(targetNode, new PingMsg()));
            this.pendingPings.put(targetNode, this.logicalTime);
        }

        // === FÁZE 3: TIMEOUTY A ŽÁDOSTI O POMOC ===
        List<String> deadNodes = new ArrayList<>();

        for (Map.Entry<String, Integer> entry : this.pendingPings.entrySet()) {
            String node = entry.getKey();
            int timeSent = entry.getValue();

            if (this.logicalTime - timeSent > 5 * this.maxDelay) {
                outbox.add(new Pair<>(disseminationProcess, new PFDMessage(node)));
                deadNodes.add(node);
            }
            else if (this.logicalTime - timeSent > 2 * this.maxDelay && !this.indirectPingSent.containsKey(node)) {
                List<String> helpers = new ArrayList<>(this.peers);
                Collections.shuffle(helpers, this.rnd);

                // Pošleme žádost až 7 náhodným uzlům
                helpers.stream().limit(7).forEach(helper -> {
                    outbox.add(new Pair<>(helper, new PingReqMsg(node)));
                });

                this.indirectPingSent.put(node, true);
            }
        }

        deadNodes.forEach(this.pendingPings::remove);

        this.logicalTime++;

        return outbox.stream().distinct().collect(Collectors.toList());
    }


    private void forwardAcks(String targetId, List<Pair<String, Message>> outbox) {
        Set<String> waiters = this.indirectWaiters.getOrDefault(targetId, Collections.emptySet());
        for (String waiter : waiters) {
            outbox.add(new Pair<>(waiter, new AckMsg(targetId)));
        }
    }
}