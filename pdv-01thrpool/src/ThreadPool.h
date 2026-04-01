#pragma once

#include <cstddef>
#include <list>
#include <vector>
#include <thread>
#include <condition_variable>
#include <mutex>

std::mutex m;            // Ochrání frontu před konflikty 🛡️
std::condition_variable cv;  // Upozorní konzumenty, že přibyla práce 🔔

template<typename JobT, typename WorkerT>
class ThreadPool {
private:
    /** Fronta uloh. */
    std::list<JobT> job_queue{};
    /** Vlakna konzumentu zpracovavajicich ulohy. */
    std::vector<std::thread> worker_threads{};

    /** Funkce, kterou maji konzumenti vykonavat. */
    WorkerT worker_fn;

public:
    ThreadPool(size_t thread_count, WorkerT worker) : worker_fn(worker) {
        // Zde vytvorte "thread_count" vlaken konzumentu:
        //   - Po spusteni bude vlakno kontrolovat, zda je ve fronte uloh "job_queue" nejaka
        //     uloha ke zpracovani, tj., fronta neni prazdna - !job_queue.empty()
        //   - Ve chvili, kdy je ve fronte uloha ke zpracovani, tak ulohu z fronty vyjme
        //     a zavola worker_fn(task), kde task je uloha ke zpracovani (priklad pouziti
        //     naleznete nize).
        //   - Vlakno se ukonci pokud uloha ke zpracovani je 0
        //   - Vytvorena vlakna vlozte do pole "worker_threads"
        for (int i = 0; i < thread_count; i++) {
                worker_threads.push_back(std::thread([this]() {
                // Ve spustenem vlakne zavolame metodu worker_loop, ktera ve smycce zpracovava vstup
                worker_loop();
            }));
        }

        // Tento kod nesplnuje zadani z nekolika duvodu:
        //   - Spousti pouze jedno vlakno konzumenta. Pokud vykonani uloh pomoci worker_fn(task)
        //     trva delsi dobu a konzument nestiha ulohy zpracovavat, zacnou se nam ulohy ve
        //     fronte hromadit. Vypocet muzeme zrychlit tim, ze si vice konzumentu podeli o
        //     ulohy ke zpracovani
        //   - Pokud vice uloh pristupuje ke fronte uloh, vysledek neni predvidatelny. Muze
        //     se nam pak stat, ze se jedna uloha zpracuje vicekrat, nebo naopak na nejakou
        //     ulohu zapomeneme. Zajistete vylucny pristup ke fronte uloh "queue" za pouziti
        //     mutexu.

        // Nezapomente, ze v kriticke sekci chceme travit co nejmene casu (tj., uvolnovat zamek
        // ihned pote, co uz ho prestaneme potrebovat). Napriklad si dejte pozor na to, abyste
        // zamek uvolnili pred tim, nez spustite vypocet ulohy pomoci worker_fn(task). Pokud by
        // totiz vypocet ulohy byl soucasti kriticke sekce, mohla by se v danou chvili pocitat
        // pouze jedna uloha - a prisli bychom o vyhody paralelizace.
    }

    void process(const JobT job) {
        // Bezpecne vlozte ulohu "job" do fronty uloh "job_queue"

        // Pridat ulohu do fronty muzeme nasledujicim volanim:
        {
            std::unique_lock<std::mutex> lk(m);
            job_queue.push_back(job);
        }
        cv.notify_one();


        // Kdyz timto zpusobem k fronte soucasne pristupuje vice vlaken, vysledek neni determinsticky.
        // Muze se se nam napriklad stat, ze nam jine vlakno nasi pridanou ulohu "prepise" a ta se tak ztrati.
        // Zabezpecte, ze k tomuto nemuze dojit.
    }

    /** Tato metoda nam umozni volajici funkci v main.cpp pockat na vsechna spustena vlakna konzumentu. */
    void join() {
        // Proiterujeme pres vsechna vlakna a pockame nez skonci.
        for (auto& worker_thread : worker_threads) {
            worker_thread.join();
        }
    }

private:
    void worker_loop() {
        while (true) {
            JobT job;
            {
                std::unique_lock<std::mutex> lock(m);

                cv.wait(lock, [this] { return !job_queue.empty(); });

                job = job_queue.front();
                job_queue.pop_front();
            }

            if (!job) {
                break; // Pokud je "zpracovana" uloha 0, skoncime
            }

            // Na zaver zavolame "worker_fn" funkci, ktera ulohu vykona
            worker_fn(job);
        }
    }
};
