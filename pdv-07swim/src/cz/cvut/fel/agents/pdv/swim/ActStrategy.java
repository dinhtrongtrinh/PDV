package cz.cvut.fel.agents.pdv.swim;

import cz.cvut.fel.agents.pdv.dsand.Message;
import cz.cvut.fel.agents.pdv.dsand.MessageWrapper;
import cz.cvut.fel.agents.pdv.dsand.Pair;

import java.util.*;
import java.util.Map.Entry;
import java.util.stream.Collectors;
import java.util.stream.IntStream;

/**
 * Trida s implementaci metody act() pro proces Failure Detector. Tuto tridu (a tridy pouzivanych zprav) budete
 * odevzdavat. Do tridy si muzete doplnit vlastni pomocne datove struktury. Hodnoty muzete inicializovat primo
 * v konstruktoru. Klicova je metoda act(), kterou vola kazda instance tridy FailureDetectorProcess ve sve metode
 * act(). Tuto metodu naimplementujte podle protokolu SWIM predstaveneho na prednasce.
 *
 * Pokud si stale jeste nevite rady s frameworkem, inspiraci muzete nalezt v resenych prikladech ze cviceni.
 */
public class ActStrategy {

    // maximalni zpozdeni zprav
    private final int maxDelayForMessages;
    private final List<String> otherProcesses;

    private int ticks_done = 0;
    private int number_of_help = 2;
    private int period = 10;

    private  Map<String, Integer> sentPings = new HashMap<>();

    private  Map<String, Integer> sentPingReqs = new HashMap<>();

    private Map<String,String> requestsToForward = new HashMap<>();
    // Definujte vsechny sve promenne a datove struktury, ktere budete potrebovat

    public ActStrategy(int maxDelayForMessages, List<String> otherProcesses,
                       int timeToDetectKilledProcess, int upperBoundOnMessages) {
        this.maxDelayForMessages = maxDelayForMessages;
        this.otherProcesses = otherProcesses;

        // Doplne inicializaci
    }

    /**
     * Metoda je volana s kazdym zavolanim metody act v F ailureDetectorProcess. Metodu implementujte-==
     * tak, jako byste implementovali metodu act() v FailureDetectorProcess, misto pouzivani send()
     * pridejte zpravy v podobe paru - prijemce, zprava do listu. Zpravy budou nasledne odeslany.
     * <p>
     * Diky zavedeni teto metody muzeme kontrolovat pocet odeslanych zprav vasi implementaci.
     */
    public List<Pair<String, Message>> act(Queue<Message> inbox, String disseminationProcess) {
        // Od DisseminationProcess muzete dostat zpravu typu DeadProcessMessage, ktera Vas
        // informuje o spravne detekovanem ukoncenem procesu.
        // DisseminationProcess muzete poslat zpravu o detekovanem "mrtvem" procesu.
        // Zprava musi byt typu PFDMessage.

        // procesy, ktery nemohu pouzit
        Set<String> cant_use = new HashSet<>();

        ticks_done++; // Zvýšíme čas hned na začátku
        List<Pair<String, Message>> outbox = new ArrayList<>();

        // checknu, jaky zpravy jsem dostal
        while(!inbox.isEmpty()){
            Message msg = inbox.poll();
            //Posila mi, ze jsem nazivu
            if(msg instanceof AckMsg){
                AckMsg ack = (AckMsg) msg;

                String ID_alive = ack.getProcessId();

                if (ID_alive == null) {
                    ID_alive = msg.sender;
                }
                sentPings.remove(ID_alive);
                sentPingReqs.remove(ID_alive);

                if(requestsToForward.containsKey(ID_alive)){
                    String from_who = requestsToForward.remove(ID_alive);
                    outbox.add(new Pair<>(from_who,msg));
                    cant_use.add(from_who);
                }

            }
            //Posila, jestli jsem nazivu
            else if(msg instanceof PingMsg){
                String kdoSePta = msg.sender;

                Message ack = new AckMsg(null);
                outbox.add(new Pair<>(kdoSePta,ack));
                cant_use.add(kdoSePta);
            }
            //Posila, jestli ten druhy je nazivu
            else if(msg instanceof PingReqMsg){
                PingReqMsg prm = (PingReqMsg) msg;

                String goal_sender = prm.getTargetProcessId();
                String from_who = msg.sender;

                outbox.add(new Pair<>(goal_sender,new PingMsg()));

                requestsToForward.put(goal_sender,from_who);
                cant_use.add(goal_sender);

            }
        }
        // checknu, jake zpravy jsou mimo tick, abych je mohl rict, ze jsou mrtvy
        List<String> toAskForHelp = new ArrayList<>();
        List<String> deadProcesses = new ArrayList<>();

        for (Map.Entry<String, Integer> entry : sentPings.entrySet()) {
            String processId = entry.getKey();
            int tickKdyBylOdeslan = entry.getValue();

            if (ticks_done - tickKdyBylOdeslan > (2 * maxDelayForMessages) + 2) {
                toAskForHelp.add(processId);
            }
        }
        for (String badProcessId : toAskForHelp) {
            sentPings.remove(badProcessId);
            sentPingReqs.put(badProcessId, ticks_done);

            List<String> possibleHelpers = new ArrayList<>(otherProcesses);
            possibleHelpers.remove(badProcessId);

            Collections.shuffle(possibleHelpers);

            int helpersCount = Math.min(number_of_help, possibleHelpers.size());
            for (int i = 0; i < helpersCount; i++) {
                String helperId = possibleHelpers.get(i);
                Message pingReq = new PingReqMsg(badProcessId);
                outbox.add(new Pair<>(helperId, pingReq));
            }
        }

        for (Map.Entry<String, Integer> entry : sentPingReqs.entrySet()) {
            String processId = entry.getKey();
            int tickKdyBylOdeslan = entry.getValue();

            if (ticks_done - tickKdyBylOdeslan > (4 * maxDelayForMessages) +2 ) {
                deadProcesses.add(processId);
            }
        }
        for (String deadId : deadProcesses) {
            sentPingReqs.remove(deadId);
            otherProcesses.remove(deadId);
            Message deadMsg = new PFDMessage(deadId);
            outbox.add(new Pair<>(disseminationProcess, deadMsg));
        }

        // vyberem nejaky proces a vratime to
        if (!otherProcesses.isEmpty() && ticks_done % period == 0) {
            List<String> availableToPing = new ArrayList<>(otherProcesses);
            availableToPing.removeAll(cant_use);
            if (!availableToPing.isEmpty()) {
                Random rand = new Random();
                String randomTarget = availableToPing.get(rand.nextInt(availableToPing.size()));

                Message newPing = new PingMsg();
                outbox.add(new Pair<>(randomTarget, newPing));

                sentPings.put(randomTarget, ticks_done);
            }
        }
        return outbox;
    }

}
