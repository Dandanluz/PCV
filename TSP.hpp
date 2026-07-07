#pragma once

#include <vector>
#include <limits>
#include <algorithm>

using namespace std;

// "cities" guarda os números (1-based) das cidades que fazem parte do problema,
// exatamente como aparecem na planilha "Cidades" (1 a 48).
// "tour" trabalha com índices LOCAIS (0..n-1) que se referem a posições dentro de "cities".
// Ou seja: a cidade real na posição i do tour é cities[tour[i]].

class TSP
{
public:
    // Distância real entre duas cidades do problema, usando seus índices locais a e b.
    static float dist(const vector<vector<float>> &matrix, const vector<int> &cities, int a, int b)
    {
        return matrix[cities[a] - 1][cities[b] - 1];
    }

    // Custo total de um ciclo (tour fechado: volta da última cidade para a primeira).
    static float tourCost(const vector<vector<float>> &matrix, const vector<int> &cities, const vector<int> &tour)
    {
        float total = 0.0f;
        int n = (int)tour.size();
        for (int i = 0; i < n; i++)
        {
            int a = tour[i];
            int b = tour[(i + 1) % n];
            total += dist(matrix, cities, a, b);
        }
        return total;
    }

    // ---------------------------------------------------------------
    // Heurística da inserção mais barata (Cheapest Insertion)
    // ---------------------------------------------------------------
    // 1) Começa com o ciclo formado pelas duas cidades mais próximas entre si.
    // 2) A cada passo, escolhe, entre todas as combinações (cidade fora do ciclo,
    //    aresta do ciclo), a que gera o MENOR custo de inserção:
    //        custo(i,k,j) = d(i,k) + d(k,j) - d(i,j)
    // 3) Insere essa cidade nessa aresta e repete até não sobrar cidade de fora.
    static vector<int> cheapestInsertion(const vector<vector<float>> &matrix, const vector<int> &cities)
    {
        int n = (int)cities.size();
        vector<int> tour;

        if (n <= 2)
        {
            for (int i = 0; i < n; i++) tour.push_back(i);
            return tour;
        }

        // Passo 1: ciclo inicial com o par de cidades mais próximo
        float best = numeric_limits<float>::max();
        int bi = 0, bj = 1;
        for (int i = 0; i < n; i++)
        {
            for (int j = i + 1; j < n; j++)
            {
                float d = dist(matrix, cities, i, j);
                if (d < best)
                {
                    best = d;
                    bi = i;
                    bj = j;
                }
            }
        }

        tour = {bi, bj};
        vector<bool> inTour(n, false);
        inTour[bi] = inTour[bj] = true;

        // Passo 2: inserir as demais n-2 cidades, uma de cada vez
        for (int step = 0; step < n - 2; step++)
        {
            float bestCost = numeric_limits<float>::max();
            int bestCity = -1, bestPos = -1;

            for (int k = 0; k < n; k++)
            {
                if (inTour[k]) continue;

                int m = (int)tour.size();
                for (int p = 0; p < m; p++)
                {
                    int a = tour[p];
                    int b = tour[(p + 1) % m];

                    float cost = dist(matrix, cities, a, k)
                               + dist(matrix, cities, k, b)
                               - dist(matrix, cities, a, b);

                    if (cost < bestCost)
                    {
                        bestCost = cost;
                        bestCity = k;
                        bestPos = p;
                    }
                }
            }

            tour.insert(tour.begin() + bestPos + 1, bestCity);
            inTour[bestCity] = true;
        }

        return tour;
    }

    // ---------------------------------------------------------------
    // Busca local 2-opt
    // ---------------------------------------------------------------
    // Testa trocar cada par de arestas (a,b) e (c,d) do ciclo por (a,c) e (b,d),
    // invertendo o trecho entre elas. Aceita a troca sempre que reduzir o custo total.
    // Repete até não haver mais melhoria (ótimo local).
    static vector<int> twoOpt(const vector<vector<float>> &matrix, const vector<int> &cities, vector<int> tour)
    {
        int n = (int)tour.size();
        if (n < 4) return tour; // 2-opt não tem o que fazer com menos de 4 cidades

        // Limite de segurança: em teoria o 2-opt sempre converge (cada troca reduz o
        // custo), mas se a matriz não for perfeitamente simétrica essa premissa quebra
        // (inverter um trecho também inverte o sentido das arestas internas, que não
        // entram no cálculo do ganho) e o algoritmo pode ficar oscilando sem nunca
        // parar. Esse limite evita que o programa trave nesse caso.
        const int MAX_PASSADAS = 500;

        bool improved = true;
        int passada = 0;
        while (improved && passada < MAX_PASSADAS)
        {
            improved = false;
            passada++;

            for (int i = 0; i < n - 1; i++)
            {
                for (int j = i + 1; j < n; j++)
                {
                    int a = tour[i];
                    int b = tour[(i + 1) % n];
                    int c = tour[j];
                    int d = tour[(j + 1) % n];

                    // arestas adjacentes ou repetidas não geram troca válida
                    if (a == c || a == d || b == c || b == d) continue;

                    float before = dist(matrix, cities, a, b) + dist(matrix, cities, c, d);
                    float after  = dist(matrix, cities, a, c) + dist(matrix, cities, b, d);

                    if (after < before - 1e-3f)
                    {
                        reverse(tour.begin() + i + 1, tour.begin() + j + 1);
                        improved = true;
                    }
                }
            }
        }
        return tour;
    }

    // ---------------------------------------------------------------
    // Busca local Or-opt
    // ---------------------------------------------------------------
    // Pega blocos de 1, 2 ou 3 cidades consecutivas e testa realocá-los
    // (na ordem original ou invertidos) em outra posição do ciclo.
    // Aceita a troca sempre que reduzir o custo total. Repete até não
    // haver mais melhoria.
    // Uma "passada" do Or-opt: procura o PRIMEIRO movimento que melhora o custo
    // (calculando só a variação nas arestas afetadas, sem recomputar o ciclo
    // inteiro) e já aplica. Retorna true se encontrou e aplicou alguma melhoria.
    static bool orOptPasso(const vector<vector<float>> &matrix, const vector<int> &cities, vector<int> &tour)
    {
        int n = (int)tour.size();

        for (int segLen = 1; segLen <= 3 && segLen < n - 2; segLen++)
        {
            for (int i = 0; i < n; i++)
            {
                vector<int> idxSeg(segLen);
                for (int t = 0; t < segLen; t++) idxSeg[t] = (i + t) % n;

                int prevIdx = (i - 1 + n) % n;
                int nextIdx = (i + segLen) % n;

                int prev = tour[prevIdx];
                int segStart = tour[idxSeg[0]];
                int segEnd = tour[idxSeg[segLen - 1]];
                int next = tour[nextIdx];

                float ganhoRemocao = dist(matrix, cities, prev, segStart)
                                    + dist(matrix, cities, segEnd, next)
                                    - dist(matrix, cities, prev, next);

                for (int k = 0; k < n; k++)
                {
                    int k2 = (k + 1) % n;

                    // pula arestas que tocam o próprio bloco ou suas bordas
                    bool sobrepoe = (k == prevIdx || k2 == nextIdx);
                    for (int t = 0; t < segLen && !sobrepoe; t++)
                        if (k == idxSeg[t] || k2 == idxSeg[t]) sobrepoe = true;
                    if (sobrepoe) continue;

                    int a = tour[k], b = tour[k2];
                    float base = dist(matrix, cities, a, b);

                    float custoFwd = dist(matrix, cities, a, segStart) + dist(matrix, cities, segEnd, b) - base;
                    float custoRev = dist(matrix, cities, a, segEnd) + dist(matrix, cities, segStart, b) - base;

                    float deltaFwd = custoFwd - ganhoRemocao;
                    float deltaRev = custoRev - ganhoRemocao;

                    if (deltaFwd < -1e-3f || deltaRev < -1e-3f)
                    {
                        bool inverter = deltaRev < deltaFwd;

                        vector<int> bloco(segLen);
                        for (int t = 0; t < segLen; t++) bloco[t] = tour[idxSeg[t]];
                        if (inverter) reverse(bloco.begin(), bloco.end());

                        vector<bool> emSeg(n, false);
                        for (int t = 0; t < segLen; t++) emSeg[idxSeg[t]] = true;

                        vector<int> resto;
                        resto.reserve(n - segLen);
                        for (int p = 0; p < n; p++)
                            if (!emSeg[p]) resto.push_back(tour[p]);

                        int posA = -1;
                        for (int p = 0; p < (int)resto.size(); p++)
                            if (resto[p] == a) { posA = p; break; }

                        resto.insert(resto.begin() + posA + 1, bloco.begin(), bloco.end());
                        tour = resto;
                        return true;
                    }
                }
            }
        }

        return false;
    }

    // Repete o Or-opt até não achar mais nenhum movimento que melhore o custo.
    static vector<int> orOpt(const vector<vector<float>> &matrix, const vector<int> &cities, vector<int> tour)
    {
        int n = (int)tour.size();
        if (n < 5) return tour; // precisa de espaço pra tirar um bloco e reinserir em outro lugar

        // Mesmo limite de segurança do twoOpt (ver comentário lá): protege contra
        // ciclos que só aconteceriam se a matriz não fosse simétrica.
        const int MAX_PASSADAS = 1000;
        int passada = 0;
        while (passada < MAX_PASSADAS && orOptPasso(matrix, cities, tour)) { passada++; }
        return tour;
    }

    // ---------------------------------------------------------------
    // Busca local por troca (Swap)
    // ---------------------------------------------------------------
    // Testa trocar de posição cada par de cidades do ciclo. Aceita a troca
    // sempre que reduzir o custo total. Repete até não haver mais melhoria.
    static vector<int> swapSearch(const vector<vector<float>> &matrix,
                              const vector<int> &cities,
                              vector<int> tour)
    {
        int n = (int)tour.size();
        if (n < 4) return tour;

        const int MAX_PASSADAS = 500;

        bool improved = true;
        int passada = 0;

        while (improved && passada < MAX_PASSADAS)
        {
            passada++;
            improved = false;

            for (int i = 0; i < n; i++)
            {
                for (int j = i + 1; j < n; j++)
                {
                    int prevI = (i - 1 + n) % n;
                    int nextI = (i + 1) % n;
                    int prevJ = (j - 1 + n) % n;
                    int nextJ = (j + 1) % n;

                    int A = tour[i], B = tour[j];
                    float delta;

                    if (nextI == j)
                    {
                        // sequência: prevI, A(i), B(j), nextJ
                        int p = tour[prevI], nx = tour[nextJ];
                        float antes  = dist(matrix, cities, p, A) + dist(matrix, cities, B, nx);
                        float depois = dist(matrix, cities, p, B) + dist(matrix, cities, A, nx);
                        delta = depois - antes;
                    }
                    else if (nextJ == i)
                    {
                        // sequência: prevJ, B(j), A(i), nextI
                        int p = tour[prevJ], nx = tour[nextI];
                        float antes  = dist(matrix, cities, p, B) + dist(matrix, cities, A, nx);
                        float depois = dist(matrix, cities, p, A) + dist(matrix, cities, B, nx);
                        delta = depois - antes;
                    }
                    else
                    {
                        float antes  = dist(matrix, cities, tour[prevI], tour[i]) + dist(matrix, cities, tour[i], tour[nextI])
                                     + dist(matrix, cities, tour[prevJ], tour[j]) + dist(matrix, cities, tour[j], tour[nextJ]);
                        float depois = dist(matrix, cities, tour[prevI], tour[j]) + dist(matrix, cities, tour[j], tour[nextI])
                                     + dist(matrix, cities, tour[prevJ], tour[i]) + dist(matrix, cities, tour[i], tour[nextJ]);
                        delta = depois - antes;
                    }

                    if (delta < -1e-3f)
                    {
                        swap(tour[i], tour[j]);
                        improved = true;
                    }
                }
            }
        }
        return tour;
    }

    // ---------------------------------------------------------------
    // Busca local combinada (estilo VND - Variable Neighborhood Descent)
    // ---------------------------------------------------------------
    // Aplica 2-opt, depois Or-opt, depois Swap; repete o ciclo enquanto
    // alguma delas ainda conseguir melhorar o custo total.
    static vector<int> localSearch(const vector<vector<float>> &matrix,
                               const vector<int> &cities,
                               vector<int> tour)
    {
        bool improved = true;
        while (improved)
        {
            float custoAntes = tourCost(matrix, cities, tour);

            tour = twoOpt(matrix, cities, tour);
            tour = orOpt(matrix, cities, tour);
            tour = swapSearch(matrix, cities, tour);

            float custoDepois = tourCost(matrix, cities, tour);
            improved = (custoDepois < custoAntes - 1e-3f);
        }

        return tour;
    }
};