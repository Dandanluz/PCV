#pragma once

#include <vector>
#include <random>
#include <algorithm>
#include "TSP.hpp"

using namespace std;

// Um indivíduo é um ciclo (tour) com índices LOCAIS, igual ao usado em TSP.hpp,
// mais o custo total já calculado (pra não recalcular sempre à toa).
struct Individuo
{
    vector<int> tour;
    float custo;
};

class MemeticoTSP
{
public:
    // Gerador de números aleatórios único e compartilhado por toda a classe.
    // (usar uma função com "static" local em vez de um membro "inline" evita
    // problemas de compatibilidade com versões mais antigas do compilador)
    static mt19937 &rng()
    {
        static mt19937 gerador(random_device{}());
        return gerador;
    }

    // -------------------------------------------------------------
    // População inicial: indivíduos aleatórios
    // -------------------------------------------------------------
    static Individuo criarAleatorio(const vector<vector<float>> &matriz, const vector<int> &cidades)
    {
        int n = (int)cidades.size();
        vector<int> tour(n);
        for (int i = 0; i < n; i++) tour[i] = i;
        shuffle(tour.begin(), tour.end(), rng());

        Individuo ind;
        ind.tour = tour;
        ind.custo = TSP::tourCost(matriz, cidades, tour);
        return ind;
    }

    static vector<Individuo> populacaoInicial(const vector<vector<float>> &matriz, const vector<int> &cidades, int tamanho)
    {
        vector<Individuo> pop;
        pop.reserve(tamanho);
        for (int i = 0; i < tamanho; i++)
            pop.push_back(criarAleatorio(matriz, cidades));
        return pop;
    }

    // -------------------------------------------------------------
    // Seleção por torneio: sorteia k indivíduos e retorna o de menor custo
    // -------------------------------------------------------------
    static int torneio(const vector<Individuo> &pop, int k)
    {
        uniform_int_distribution<int> dist(0, (int)pop.size() - 1);
        int melhor = dist(rng());
        for (int i = 1; i < k; i++)
        {
            int cand = dist(rng());
            if (pop[cand].custo < pop[melhor].custo) melhor = cand;
        }
        return melhor;
    }

    // -------------------------------------------------------------
    // Crossover OX (Order Crossover): copia um trecho do pai 1 e completa
    // com as cidades do pai 2 na ordem em que aparecem, pulando repetidas.
    // -------------------------------------------------------------
    static vector<int> crossoverOX(const vector<int> &pai1, const vector<int> &pai2)
    {
        int n = (int)pai1.size();
        uniform_int_distribution<int> dist(0, n - 1);
        int a = dist(rng()), b = dist(rng());
        if (a > b) swap(a, b);

        vector<int> filho(n, -1);
        vector<bool> usado(n, false);

        for (int i = a; i <= b; i++)
        {
            filho[i] = pai1[i];
            usado[pai1[i]] = true;
        }

        int pos = (b + 1) % n;
        for (int i = 0; i < n; i++)
        {
            int cidade = pai2[(b + 1 + i) % n];
            if (!usado[cidade])
            {
                filho[pos] = cidade;
                usado[cidade] = true;
                pos = (pos + 1) % n;
            }
        }

        return filho;
    }

    // Mutação simples: com probabilidade "taxa", troca duas cidades de posição
    static void mutacaoSwap(vector<int> &tour, float taxa)
    {
        uniform_real_distribution<float> prob(0.0f, 1.0f);
        if (prob(rng()) < taxa)
        {
            uniform_int_distribution<int> dist(0, (int)tour.size() - 1);
            int i = dist(rng()), j = dist(rng());
            swap(tour[i], tour[j]);
        }
    }

    // -------------------------------------------------------------
    // Loop principal do algoritmo memético
    // -------------------------------------------------------------
    // tamanhoPopulacao : quantos indivíduos por geração
    // geracoes         : número de gerações
    // torneioK         : quantos competem em cada torneio de seleção
    // taxaMutacao      : probabilidade de mutação por filho gerado
    // probBuscaLocal   : probabilidade de aplicar a busca local (2-opt+Or-opt+Swap)
    //                    em cada filho gerado (1.0 = memético "puro", sempre refina)
    static Individuo executar(
        const vector<vector<float>> &matriz,
        const vector<int> &cidades,
        int tamanhoPopulacao,
        int geracoes,
        int torneioK,
        float taxaMutacao,
        float probBuscaLocal)
    {
        vector<Individuo> pop = populacaoInicial(matriz, cidades, tamanhoPopulacao);

        // Já refina a população inicial (é o que caracteriza o algoritmo como memético)
        for (auto &ind : pop)
        {
            ind.tour = TSP::localSearch(matriz, cidades, ind.tour);
            ind.custo = TSP::tourCost(matriz, cidades, ind.tour);
        }

        Individuo melhorGlobal = *min_element(pop.begin(), pop.end(),
            [](const Individuo &a, const Individuo &b) { return a.custo < b.custo; });

        uniform_real_distribution<float> prob(0.0f, 1.0f);

        for (int g = 0; g < geracoes; g++)
        {
            vector<Individuo> novaPop;
            novaPop.reserve(tamanhoPopulacao);
            novaPop.push_back(melhorGlobal); // elitismo: o melhor de todos nunca se perde

            while ((int)novaPop.size() < tamanhoPopulacao)
            {
                int i1 = torneio(pop, torneioK);
                int i2 = torneio(pop, torneioK);

                vector<int> filhoTour = crossoverOX(pop[i1].tour, pop[i2].tour);
                mutacaoSwap(filhoTour, taxaMutacao);

                if (prob(rng()) < probBuscaLocal)
                    filhoTour = TSP::localSearch(matriz, cidades, filhoTour);

                Individuo filho;
                filho.tour = filhoTour;
                filho.custo = TSP::tourCost(matriz, cidades, filhoTour);

                novaPop.push_back(filho);
            }

            pop = novaPop;

            Individuo melhorGeracao = *min_element(pop.begin(), pop.end(),
                [](const Individuo &a, const Individuo &b) { return a.custo < b.custo; });

            if (melhorGeracao.custo < melhorGlobal.custo)
                melhorGlobal = melhorGeracao;
        }

        return melhorGlobal;
    }
};
