#include "sort.h"
#include <limits>
#include <vector>
#include <atomic>
#include <omp.h>

#include <stdexcept>

// implementace vaseho radiciho algoritmu. Detalnejsi popis zadani najdete v "sort.h"
void radix_par(std::vector<std::string*>& vector_to_sort, MappingFunction mapping_function,
               size_t alphabet_size, size_t str_size) {

    // sem prijde vase implementace. zakomentujte tuto radku

    // abeceda se nemeni. jednotlive buckety by mely reprezentovat znaky teto abecedy. poradi znaku v abecede
    // dostanete volanim funkce mappingFunction nasledovne: mappingFunction((*p_retezec).at(poradi_znaku))

    // vytvorte si spravnou reprezentaci bucketu, kam budete retezce umistovat

    // pro vetsi jednoduchost uvazujte, ze vsechny retezce maji stejnou delku - string_lengths. nemusite tedy resit
    // zadne krajni pripady

    // na konci metody by melo byt zaruceno, ze vector pointeru - vector_to_sort bude spravne serazeny.
    // pointery budou serazeny podle retezcu, na ktere odkazuji, kdy retezcu jsou serazeny abecedne


    int pocet_vlaken = omp_get_max_threads();
    auto vector_size = vector_to_sort.size();
    for (int i = str_size - 1; 0 <= i; i--) {
        std::vector<std::vector<std::string*>>bucket(alphabet_size);
        std::vector<size_t> prefix_sum(alphabet_size,0);

        // davani vsechno do "bucketu"
        std::vector<std::vector<std::vector<std::string*>>> thread_buckets(
            pocet_vlaken,
            std::vector<std::vector<std::string*>>(alphabet_size)
        );

        #pragma omp parallel for schedule(static)
        for (size_t j = 0; j < vector_size; j++) {
            int id_vlakna = omp_get_thread_num();
            size_t position = mapping_function(vector_to_sort[j]->at(i));
            thread_buckets[id_vlakna][position].push_back(vector_to_sort[j]);
        }


        for (size_t znak = 0; znak < alphabet_size; znak++) {
            for (int vlakno = 0; vlakno < pocet_vlaken; vlakno++) {
                auto& lokalni_kbelik = thread_buckets[vlakno][znak];
                if (!lokalni_kbelik.empty()) {
                    bucket[znak].insert(
                        bucket[znak].end(),
                        lokalni_kbelik.begin(),
                        lokalni_kbelik.end()
                    );
                }
            }
        }


        // pocitani prefixsum
        prefix_sum[0] = 0;
        for (size_t j = 1; j < alphabet_size; j++) {
            prefix_sum[j] += bucket[j-1].size() + prefix_sum[j-1];
        }

        //ted to dat do vector_to_sort

        #pragma omp parallel for schedule(dynamic, 64)
        for (size_t u = 0; u < bucket.size(); u++) {
            auto one_bucket = bucket[u];
            if (one_bucket.size() > 0) {
                size_t offset = prefix_sum[u];
                for (size_t j = 0; j < one_bucket.size(); j++) {
                    vector_to_sort[offset+j] = one_bucket[j];
                }
            }
        }
        bucket.clear();
    }
}