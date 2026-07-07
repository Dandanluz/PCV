#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <string>
#include <chrono>
#include <limits>
#include "CSVreader.hpp"
#include "TSP.hpp"
#include "Memetico.hpp"

using namespace std;

struct Problema
{
    string nome;
    string medida;
    vector<int> cidades;
};

vector<int> intervalo(int de, int ate)
{
    vector<int> v;
    for (int i = de; i <= ate; i++) v.push_back(i);
    return v;
}

int main()
{
    CSVREADER reader;
    vector<vector<float>> matrizKm = reader.openCSV("PCV_ Matriz do problema.xlsx - Km.csv");
    vector<vector<float>> matrizMin = reader.openCSV("PCV_ Matriz do problema.xlsx - Min.csv");

    if (matrizKm.empty() || matrizMin.empty())
    {
        cerr << "Erro ao carregar as matrizes. Verifique os caminhos dos arquivos." << endl;
        return 1;
    }

    vector<Problema> problemas = {
        {"Problema 1",  "Km",  intervalo(1, 48)},
        {"Problema 2",  "Min", intervalo(1, 48)},
        {"Problema 3",  "Km",  intervalo(1, 36)},
        {"Problema 4",  "Min", intervalo(1, 36)},
        {"Problema 5",  "Km",  intervalo(1, 24)},
        {"Problema 6",  "Min", intervalo(1, 24)},
        {"Problema 7",  "Km",  intervalo(1, 12)},
        {"Problema 8",  "Min", intervalo(1, 12)},
        {"Problema 9",  "Km",  {1, 7, 8, 9, 10, 11, 12}},
        {"Problema 10", "Min", {1, 7, 8, 9, 10, 11, 12}},
        {"Problema 11", "Km",  intervalo(1, 6)},
        {"Problema 12", "Min", intervalo(1, 6)},
    };

    const int TAMANHO_POPULACAO  = 40;
    const int GERACOES           = 150;
    const int TORNEIO_K          = 3;
    const float TAXA_MUTACAO     = 0.15f;
    const float PROB_BUSCA_LOCAL = 1.0f;
    const int NUM_EXECUCOES      = 20;

    ofstream arquivo("resultados_memetico.txt");
    if (!arquivo.is_open())
    {
        cerr << "Nao foi possivel criar o arquivo de resultados." << endl;
        return 1;
    }

    arquivo << "RESULTADOS - ALGORITMO MEMETICO (PCV)\n";
    arquivo << "Cada instancia foi executada " << NUM_EXECUCOES << " vezes.\n";
    arquivo << "Parametros: populacao=" << TAMANHO_POPULACAO
            << ", geracoes=" << GERACOES
            << ", torneio_k=" << TORNEIO_K
            << ", taxa_mutacao=" << TAXA_MUTACAO << "\n";
    arquivo << string(70, '=') << "\n\n";

    arquivo << fixed << setprecision(2);
    cout << fixed << setprecision(2);

    for (const auto &prob : problemas)
    {
        const vector<vector<float>> &matriz = (prob.medida == "Km") ? matrizKm : matrizMin;

        float menorCusto = numeric_limits<float>::max();
        float somaCustos = 0.0f;
        double somaTempos = 0.0;
        vector<int> melhorTourLocal;

        cout << "Executando " << prob.nome << " (" << prob.medida << ", n=" << prob.cidades.size() << ")...\n";
        cout.flush();

        for (int exec = 1; exec <= NUM_EXECUCOES; exec++)
        {
            auto inicio = chrono::steady_clock::now();
            Individuo resultado = MemeticoTSP::executar(
                matriz, prob.cidades,
                TAMANHO_POPULACAO, GERACOES, TORNEIO_K,
                TAXA_MUTACAO, PROB_BUSCA_LOCAL);
            auto fim = chrono::steady_clock::now();
            double segundos = chrono::duration<double>(fim - inicio).count();

            somaCustos += resultado.custo;
            somaTempos += segundos;

            if (resultado.custo < menorCusto)
            {
                menorCusto = resultado.custo;
                melhorTourLocal = resultado.tour;
            }

            cout << "  execucao " << exec << "/" << NUM_EXECUCOES
                 << " - custo: " << resultado.custo
                 << " - tempo: " << segundos << "s\n";
            cout.flush();
        }

        float custoMedio = somaCustos / NUM_EXECUCOES;
        double tempoMedio = somaTempos / NUM_EXECUCOES;

        arquivo << prob.nome << " (" << prob.medida << ", n=" << prob.cidades.size() << ")\n";
        arquivo << string(70, '-') << "\n";
        arquivo << "Menor custo encontrado (das " << NUM_EXECUCOES << " execucoes): " << menorCusto << "\n";
        arquivo << "Custo medio:                                     " << custoMedio << "\n";
        arquivo << "Tempo medio de execucao:                         " << tempoMedio << " s\n";
        arquivo << "Melhor rota encontrada: ";
        for (int idx : melhorTourLocal) arquivo << prob.cidades[idx] << " ";
        if (!melhorTourLocal.empty()) arquivo << prob.cidades[melhorTourLocal[0]];
        arquivo << "\n\n";

        cout << "  -> menor custo: " << menorCusto
             << " | medio: " << custoMedio
             << " | tempo medio: " << tempoMedio << "s\n\n";
        cout.flush();
    }

    arquivo.close();
    cout << "Resumo gravado em resultados_memetico.txt\n";

    return 0;
}
