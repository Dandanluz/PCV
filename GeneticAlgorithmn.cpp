#include "CSVreader.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <numeric>   
#include <random>    
#include <algorithm> 
#include <iomanip> 
#include <chrono>
#include <cmath>

using namespace std;
using namespace std::chrono;

class GeneticAlgorithmn
{
    private:
        CSVREADER csv;
        vector<vector<float>> adjacencyMatrixKm;
        vector<vector<float>> adjacencyMatrixMin;
        mt19937 rng{random_device{}()};

    public:
        // Construtor padrão: carrega a matriz completa do CSV
        GeneticAlgorithmn() 
        {

            adjacencyMatrixKm = csv.openCSV("PCV_ Matriz do problema.xlsx - Km.csv");
            adjacencyMatrixMin = csv.openCSV("PCV_ Matriz do problema.xlsx - Min.csv"); 
        }

        GeneticAlgorithmn(const vector<vector<float>>& km, const vector<vector<float>>& min): adjacencyMatrixKm(km),adjacencyMatrixMin(min){}
        
        vector<vector<int>> PopulationGenerator(int populationSize)
        {
            vector<vector<int>> populationSet;
            int numCities = adjacencyMatrixKm.size();
            
            if (numCities == 0) return populationSet;

            vector<int> route(numCities);
            iota(route.begin(), route.end(), 0);
            

            for(int count = 0; count < populationSize; count++)
            {
                shuffle(route.begin(), route.end(), rng);
                populationSet.push_back(route);
            }
                
            return populationSet;
        }

        
        float DistanceCalculator(const vector<int>& route, bool isMinutes)
        {
            float total = 0;

            const auto& matrix =
                isMinutes ? adjacencyMatrixMin : adjacencyMatrixKm;

            int n = route.size();

            for(int i = 0; i < n; i++)
            {
                int current = route[i];
                int next = route[(i + 1) % n];

                total += matrix[current][next];
            }

            return total;
        }

        vector<float> ReturnFitness(const vector<vector<int>>& populationSet, bool isMinutes)
        {
            vector<float> fitnessScores(populationSet.size());

            for(size_t i = 0; i < populationSet.size(); i++)
            {
                float distance = DistanceCalculator(populationSet[i], isMinutes);
                fitnessScores[i] = 1.0f / (distance + 0.0001f);
            }

            return fitnessScores;
        }

        int getBestIndividualIndex(const vector<vector<int>>& populationSet,bool isMinutes)
        {
            int bestIndex = 0;
            float minDistance = DistanceCalculator(populationSet[0], isMinutes);

            for (size_t i = 1; i < populationSet.size(); i++)
            {
                float currentDistance = DistanceCalculator(populationSet[i], isMinutes);
                if (currentDistance < minDistance)
                {
                    minDistance = currentDistance;
                    bestIndex = i;
                }
            }
            return bestIndex;
        }

        vector<int> selectionTournament(const vector<vector<int>>& populationSet,  bool isMinutes, int tournamentSize = 5)
        {
            uniform_int_distribution<int> dist(0, populationSet.size() - 1);

            int bestIdx = dist(rng);
            float minDistance = DistanceCalculator(populationSet[bestIdx], isMinutes);

            for (int i = 1; i < tournamentSize; i++)
            {
                int currentIdx = dist(rng);
                float currentDistance = DistanceCalculator(populationSet[currentIdx], isMinutes);
                if (currentDistance < minDistance)
                {
                    minDistance = currentDistance;
                    bestIdx = currentIdx;
                }
            }
            return populationSet[bestIdx];
        }

        vector<int> orderedCrossover(const vector<int>& parent1, const vector<int>& parent2)
        {
            int numCities = parent1.size();
            vector<int> child(numCities, -1);
            vector<bool> used(numCities, false);

            uniform_int_distribution<int> dist(0, numCities - 1);

            int cut1 = dist(rng);
            int cut2 = dist(rng);

            if (cut1 > cut2)
                swap(cut1, cut2);

            // Copia o segmento do primeiro pai
            for (int i = cut1; i <= cut2; i++)
            {
                child[i] = parent1[i];
                used[parent1[i]] = true;
            }

            int currentChildPos = (cut2 + 1) % numCities;

            // Completa com genes do segundo pai
            for (int i = 0; i < numCities; i++)
            {
                int parent2Pos = (cut2 + 1 + i) % numCities;
                int gene = parent2[parent2Pos];

                if (!used[gene])
                {
                    child[currentChildPos] = gene;
                    used[gene] = true;
                    currentChildPos = (currentChildPos + 1) % numCities;
                }
            }

            return child;
        }

        void applyMutation(vector<int>& route, float mutationRate)
        {
            uniform_real_distribution<float> distPercent(0.0f, 1.0f);

            if (distPercent(rng) < mutationRate)
            {
                int numCities = route.size();
                uniform_int_distribution<int> distIndex(0, numCities - 1);

                int cut1 = distIndex(rng);
                int cut2 = distIndex(rng);

                if (cut1 > cut2)
                {
                    swap(cut1, cut2);
                }

                while (cut1 < cut2)
                {
                    swap(route[cut1], route[cut2]);
                    cut1++;
                    cut2--;
                }
            }
        }

        pair<float, vector<int>> runEvolution(int populationSize, int maxGenerations, float baseMutationRate, bool isMinutes, bool verbose = false)
        {
            vector<vector<int>> population = PopulationGenerator(populationSize);
            if(population.empty()) return {0.0f, {}};

            int initialBestIdx = getBestIndividualIndex(population, isMinutes);
            if (verbose) cout << "Distancia Inicial: " << DistanceCalculator(population[initialBestIdx], isMinutes) << endl;

            float bestDistanceEver = DistanceCalculator(population[initialBestIdx], isMinutes);
            vector<int> bestRouteEver = population[initialBestIdx];

            int stagnationCounter = 0;
            float mutationRate = baseMutationRate;
            const float immigrantFraction = 0.2f;

            for (int g = 1; g <= maxGenerations; g++)
            {
                vector<vector<int>> nextPopulation;

                int bestIdx = getBestIndividualIndex(population, isMinutes);
                float currentBestDistance = DistanceCalculator(population[bestIdx], isMinutes);

                if (currentBestDistance < bestDistanceEver)
                {
                    bestDistanceEver = currentBestDistance;
                    bestRouteEver = population[bestIdx];
                    stagnationCounter = 0;
                    mutationRate = baseMutationRate;
                }
                else
                {
                    stagnationCounter++;
                }

                nextPopulation.push_back(bestRouteEver);

                bool triggerDiversity = false;
                if (stagnationCounter > 0 && stagnationCounter % 50 == 0)
                {
                    mutationRate = min(mutationRate * 1.5f, 0.3f);
                    triggerDiversity = true;
                    if (verbose)
                        cout << "  [Estagnacao na geracao " << g 
                            << " | Nova taxa de mutacao: " << (mutationRate * 100) 
                            << "% | Injetando imigrantes]" << endl;
                }

                int numImmigrants = triggerDiversity ? (int)(populationSize * immigrantFraction) : 0;

                vector<vector<int>> forcedChildren;
                if (numImmigrants > 0)
                {
                    vector<vector<int>> immigrants = PopulationGenerator(numImmigrants);
                    for (auto& immigrant : immigrants)
                    {
                        vector<int> goodParent = selectionTournament(population, isMinutes, 3);
                        vector<int> hybridChild = orderedCrossover(immigrant, goodParent);
                        applyMutation(hybridChild, mutationRate);
                        forcedChildren.push_back(hybridChild);
                    }
                }

                int numToBreed = populationSize - 1 - (int)forcedChildren.size();

                while (nextPopulation.size() < (size_t)(1 + numToBreed))
                {
                    vector<int> p1 = selectionTournament(population, isMinutes,3);
                    vector<int> p2 = selectionTournament(population, isMinutes,3);

                    vector<int> child = orderedCrossover(p1, p2);
                    applyMutation(child, mutationRate);

                    nextPopulation.push_back(child);
                }

                for (auto& fc : forcedChildren) nextPopulation.push_back(fc);

                population = nextPopulation;

                if (verbose && (g % 100 == 0 || g == maxGenerations))
                {
                    cout << "Geracao " << g << " | Melhor Distancia: " << bestDistanceEver << endl;
                }
            }

            return {bestDistanceEver, bestRouteEver};
        }
};

// ------------------------------------------------------------------
// Estrutura de instância de teste, igual ao NN.cpp: mesmo conjunto de
// cidades pode ser interpretado em Km ou em minutos (isMinutos define
// só a FORMATAÇÃO da saída, o algoritmo roda igual nos dois casos).
// ------------------------------------------------------------------
struct TestCase
{
    string nome;
    vector<int> city;
    bool isMinutos;
};

// Extrai a submatriz correspondente às cidades do teste (mesma lógica do NN.cpp)
vector<vector<float>> buildSubMatrix(const vector<vector<float>>& adjacencyMatrix, const vector<int>& cities)
{
    int size = cities.size();
    vector<vector<float>> subMatrix(size, vector<float>(size));

    for (int i = 0; i < size; i++)
        for (int j = 0; j < size; j++)
            subMatrix[i][j] = adjacencyMatrix[cities[i]][cities[j]];

    return subMatrix;
}

struct ResultsExecutation
{
    int NumberExecutation;
    float BestDist;
    double TimeInSeconds;
    vector<int> melhorRota;
};

void salvarResumoTXT(const string& caminhoArquivo,
                      const string& nomeInstancia,
                      int populationSize,
                      int maxGenerations,
                      float mutationRate,
                      bool isMinutos,
                      const vector<ResultsExecutation>& results)
{
    ofstream arquivo(caminhoArquivo, ios::app);
    if (!arquivo.is_open())
    {
        cerr << "ERRO: nao foi possivel abrir " << caminhoArquivo << " para escrita." << endl;
        return;
    }

    float menorDistancia = results[0].BestDist;
    float somaDistancias = 0.0f;
    double somaTempos = 0.0;
    int execucaoDoMenor = results[0].NumberExecutation;

    for (const auto& r : results)
    {
        somaDistancias += r.BestDist;
        somaTempos += r.TimeInSeconds;
        if (r.BestDist < menorDistancia)
        {
            menorDistancia = r.BestDist;
            execucaoDoMenor = r.NumberExecutation;
        }
    }

    float distanciaMedia = somaDistancias / results.size();
    double tempoMedio = somaTempos / results.size();

    arquivo << fixed << setprecision(2);
    arquivo << "==========================================================" << endl;
    arquivo << "Instancia: " << nomeInstancia << endl;
    arquivo << "Algoritmo: Algoritmo Genetico puro" << endl;
    arquivo << "Parametros: populacao=" << populationSize 
            << " | geracoes=" << maxGenerations 
            << " | taxaMutacaoBase=" << (mutationRate * 100) << "%" << endl;
    arquivo << "Numero de execucoes: " << results.size() << endl;
    arquivo << "----------------------------------------------------------" << endl;
    arquivo << "Menor valor encontrado: " << menorDistancia
            << " (execucao #" << execucaoDoMenor << ")" << endl;
    arquivo << "Valor medio das execucoes: " << menorDistancia << endl;
    arquivo << "Tempo medio de execucao: " << tempoMedio << " segundos" << endl;
    arquivo << "==========================================================" << endl;
    arquivo << endl;

    arquivo.close();
}

void salvarDetalhesCSV(const string& caminhoArquivo, const string& nomeInstancia,
                        const ResultsExecutation& r)
{
    ifstream teste(caminhoArquivo);
    bool existe = teste.good();
    teste.close();

    ofstream arquivo(caminhoArquivo, ios::app);
    if (!arquivo.is_open()) return;

    if (!existe)
        arquivo << "instancia,execucao,bestDistance,tempoSegundos,rota" << endl;

    arquivo << nomeInstancia << "," << r.NumberExecutation << ","
            << fixed << setprecision(2) << r.BestDist << ","
            << r.TimeInSeconds << ",\"";

    for (size_t i = 0; i < r.melhorRota.size(); i++)
    {
        arquivo << r.melhorRota[i];
        if (i != r.melhorRota.size() - 1) arquivo << " ";
    }
    arquivo << "\"" << endl;

    arquivo.close();
}

// Roda as 20 execuções exigidas pelo enunciado para UMA instância
void rodarInstancia(const string& NameI,  const vector<vector<float>>& matrixKm, const vector<vector<float>>& matrixMin, bool isMinutes,int populationSize, int maxGenerations, float mutationRate,const string& pathTXT, const string& pathDetailsCSV)
{
    vector<ResultsExecutation> results;
    const int NUM_EXECUCOES = 20;

    GeneticAlgorithmn ga(matrixKm, matrixMin);

    cout << "Rodando problema: \n" << NameI << endl;

    for (int i = 1; i <= NUM_EXECUCOES; i++)
    {
        auto inicio = high_resolution_clock::now();
        auto evolutionResult = ga.runEvolution(populationSize, maxGenerations, mutationRate, isMinutes, false);
        auto fim = high_resolution_clock::now();

        double tempoSegundos = duration_cast<duration<double>>(fim - inicio).count();

        ResultsExecutation r;
        r.NumberExecutation = i;
        r.BestDist = evolutionResult.first;
        r.melhorRota = evolutionResult.second;
        r.TimeInSeconds = tempoSegundos;

        results.push_back(r);
        salvarDetalhesCSV(pathDetailsCSV, NameI, r);

        cout << "  Execucao " << i << "/" << NUM_EXECUCOES << " | Distancia: " << r.BestDist << " | Tempo: " << fixed << setprecision(2) << r.TimeInSeconds << "s" << endl;
    }

    salvarResumoTXT(pathTXT, NameI, populationSize, maxGenerations, mutationRate, isMinutes, results);

}

int main() 
{
    cout << fixed << setprecision(2);

    int populationSize = 500;
    int maxGenerations = 1000;
    float mutationRate = 0.06f;

    const string caminhoResumoTXT = "resumo_GA.txt";
    const string caminhoDetalhesCSV = "detalhes_GA.csv";

    CSVREADER csv;
   
vector<vector<float>> adjacencyMatrixKm =csv.openCSV("PCV_ Matriz do problema.xlsx - Km.csv");

vector<vector<float>> adjacencyMatrixMin =csv.openCSV("PCV_ Matriz do problema.xlsx - Min.csv");

    vector<TestCase> tests =
    {
        {"Problema 1 (Km)",  {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47}, false},
        {"Problema 2 (min)", {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47}, true},

        {"Problema 3 (Km)",  {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35}, false},
        {"Problema 4 (min)", {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35}, true},

        {"Problema 5 (Km)",  {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23}, false},
        {"Problema 6 (min)", {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23}, true},

        {"Problema 7 (Km)",  {0,1,2,3,4,5,6,7,8,9,10,11}, false},
        {"Problema 8 (min)", {0,1,2,3,4,5,6,7,8,9,10,11}, true},

        {"Problema 9 (Km)",  {0,6,7,8,9,10,11}, false},
        {"Problema 10 (min)",{0,6,7,8,9,10,11}, true},

        {"Problema 11 (Km)", {0,1,2,3,4,5}, false},
        {"Problema 12 (min)",{0,1,2,3,4,5}, true}
    };

    for (const auto& test : tests)
    {
        vector<vector<float>> subMatrixKm = buildSubMatrix(adjacencyMatrixKm, test.city);

        vector<vector<float>> subMatrixMin = buildSubMatrix(adjacencyMatrixMin, test.city);  

        rodarInstancia(test.nome, subMatrixKm, subMatrixMin, test.isMinutos,populationSize, maxGenerations, mutationRate, caminhoResumoTXT, caminhoDetalhesCSV);
    }

    cout << "\nProcesso concluido. Resumo em '" << caminhoResumoTXT 
         << "', detalhes em '" << caminhoDetalhesCSV << "'." << endl;

    return 0;
}