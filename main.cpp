#include <iostream>
#include <vector>
#include <string>
#include <numeric>
#include "CSVreader.hpp"
#include "TSP.hpp"

using namespace std;

struct Problema
{
    string nome;
    string medida;      // "Km" ou "Min"
    vector<int> cidades; // números das cidades (1-based), como na planilha "Cidades"
};

vector<int> intervalo(int de, int ate)
{
    vector<int> v;
    for (int i = de; i <= ate; i++) v.push_back(i);
    return v;
}

int main()
{
    // --- 1) Carregar as duas matrizes completas (48x48) uma única vez ---
    CSVREADER reader;
    vector<vector<float>> matrizKm = reader.openCSV("PCV_ Matriz do problema.xlsx - Km.csv");
    vector<vector<float>> matrizMin = reader.openCSV("PCV_ Matriz do problema.xlsx - Min.csv");

    if (matrizKm.empty() || matrizMin.empty())
    {
        cerr << "Erro ao carregar as matrizes. Verifique os caminhos dos arquivos." << endl;
        return 1;
    }

    // --- 2) Definir os 12 problemas ---
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

    // --- 3) Resolver cada problema: inserção mais barata + busca local (2-opt) ---
    for (const auto &prob : problemas)
    {
        const vector<vector<float>> &matriz = (prob.medida == "Km") ? matrizKm : matrizMin;

        vector<int> tourInicial = TSP::cheapestInsertion(matriz, prob.cidades);
        float custoInicial = TSP::tourCost(matriz, prob.cidades, tourInicial);

        vector<int> tourFinal = TSP::twoOpt(matriz, prob.cidades, tourInicial);
        float custoFinal = TSP::tourCost(matriz, prob.cidades, tourFinal);

        cout << "===== " << prob.nome << " (" << prob.medida << ", n=" << prob.cidades.size() << ") =====\n";

        cout << "Rota (insercao mais barata): ";
        for (int idx : tourInicial) cout << prob.cidades[idx] << " ";
        cout << prob.cidades[tourInicial[0]] << "\n";
        cout << "Custo inicial: " << custoInicial << "\n";

        cout << "Rota (apos 2-opt): ";
        for (int idx : tourFinal) cout << prob.cidades[idx] << " ";
        cout << prob.cidades[tourFinal[0]] << "\n";
        cout << "Custo apos busca local: " << custoFinal << "\n\n";
    }

    return 0;
}
