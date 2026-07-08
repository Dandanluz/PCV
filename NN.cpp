#include <iostream>
#include <vector>
#include <climits>
#include <algorithm>
#include "CSVreader.hpp"
#include <iomanip>
#include <cmath> 

struct TestCase
{
    string nome;
    vector<int> city;
};

using namespace std;

class NN
{
public:
    CSVREADER csv;

    vector<int> NeighborNearest(int actual_city, int size, const vector<vector<float>>& adjacencyMatrixKm) //Algoritmo de busca do vizinho mais próximo
    {
        vector<bool> visited(adjacencyMatrixKm.size(), false);
        vector<int> route;

        int current = actual_city;
        visited[current] = true;
        route.push_back(current);

        while (!all_of(visited.begin(), visited.end(), [](bool v) { return v; }))
        {
            float dist_min = numeric_limits<float>::max();
            int next = -1;

            for (int i = 0; i < (int)adjacencyMatrixKm.size(); i++)
            {
                if (!visited[i] && adjacencyMatrixKm[current][i] < dist_min)
                {
                    dist_min = adjacencyMatrixKm[current][i];
                    next = i;
                }
            }

            if (next == -1) break;

            route.push_back(next);
            visited[next] = true;
            current = next;
        }

        return route;
    }
};
NN nn;
// Calcula a distância total de uma rota fechada
float TotalDistCalculator(const vector<int>& route, const vector<vector<float>>& matrix)
{
    float total = 0;

    for (int i = 0; i < route.size(); i++)
    {
        int current = route[i];
        int next = route[(i + 1) % route.size()];

        total += matrix[current][next];
    }

    return total;
}


// Remove 2 arestas e reconecta invertendo o segmento entre elas
bool twoOpt(vector<int>& route, const vector<vector<float>>& adjacencyMatrixKm)
{
    bool improvedAny = false;
    bool improved = true;
    int n = route.size();

    while (improved)
    {
        improved = false;

        for (int i = 0; i < n - 1; i++)
        {
            for (int j = i + 1; j < n; j++)
            {
                int a = route[i];

                int b = route[(i + 1) % n];

                int c = route[j];

                int d = route[(j + 1) % n];

                if (a == c || a == d || b == c) continue;


                float actualDist = adjacencyMatrixKm[a][b] + adjacencyMatrixKm[c][d];
                float newDist    = adjacencyMatrixKm[a][c] + adjacencyMatrixKm[b][d];

                if (newDist < actualDist - 1e-6f)
                {
                    reverse(route.begin() + i + 1, route.begin() + j + 1);
                    improved = true;
                    improvedAny = true;
                }
            }
        }
    }
    return improvedAny;
}



bool swapLocalSearch(vector<int>& route, const vector<vector<float>>& adjacencyMatrixKm)// Troca a posição de dois vértices na sequência da rota
{
    bool improvedAny = false;
    bool improved = true;
    int n = route.size();

    while (improved)
    {
        improved = false;

        for (int i = 0; i < n; i++)
        {
            for (int j = i + 1; j < n; j++)
            {

                int prevI = route[(i - 1 + n) % n];
                int nextI = route[(i + 1) % n];


                int prevJ = route[(j - 1 + n) % n];
                int nextJ = route[(j + 1) % n];

                int cityI = route[i];
                int cityJ = route[j];

                if (nextI == cityJ || nextJ == cityI)
                {
                    swap(route[i], route[j]);
                    float depois = TotalDistCalculator(route, adjacencyMatrixKm);
                    swap(route[i], route[j]);
                    float antes = TotalDistCalculator(route, adjacencyMatrixKm);

                    if (depois < antes - 1e-6f)
                    {
                        swap(route[i], route[j]);
                        improved = true;
                        improvedAny = true;
                    }
                    continue;
                }


                float PreviousCost = adjacencyMatrixKm[prevI][cityI] + adjacencyMatrixKm[cityI][nextI] + adjacencyMatrixKm[prevJ][cityJ] + adjacencyMatrixKm[cityJ][nextJ];


                float CostAfter = adjacencyMatrixKm[prevI][cityJ] + adjacencyMatrixKm[cityJ][nextI] + adjacencyMatrixKm[prevJ][cityI] + adjacencyMatrixKm[cityI][nextJ];

                if (CostAfter < PreviousCost - 1e-6f)
                {
                    swap(route[i], route[j]);
                    improved = true;
                    improvedAny = true;
                }
            }
        }
    }
    return improvedAny;
}


bool Shift(vector<int>& route, const vector<vector<float>>& adjacencyMatrixKm)// Remove uma cidade de uma posição e faz o remajenamento para outra posição da rota
{
    bool improvedAny = false;
    bool improved = true;
    int n = route.size();

    while (improved)
    {
        improved = false;

        for (int i = 0; i < n; i++)
        {
            int city = route[i];
            int prevI = route[(i - 1 + n) % n];
            int nextI = route[(i + 1) % n];

    
            float DeleteCost = adjacencyMatrixKm[prevI][city] + adjacencyMatrixKm[city][nextI]
                                - adjacencyMatrixKm[prevI][nextI];

            for (int j = 0; j < n; j++)
            {
                if (j == i || j == (i - 1 + n) % n) continue; 

                int destA = route[j];
                int destB = route[(j + 1) % n];

               
                float CostOfInput = adjacencyMatrixKm[destA][city] + adjacencyMatrixKm[city][destB]
                                    - adjacencyMatrixKm[destA][destB];

                if (CostOfInput < DeleteCost - 1e-6f)
                {
                    vector<int> newRoute = route;
                    newRoute.erase(newRoute.begin() + i);

                    int AfterInput = (j > i) ? j : j + 1;
                    newRoute.insert(newRoute.begin() + AfterInput, city);

                    route = newRoute;
                    improved = true;
                    improvedAny = true;
                    break;
                }
            }
            if (improved) break;
        }
    }
    return improvedAny;
}

bool Inversion(vector<int>& route, const vector<vector<float>>& adjacencyMatrixKm)
{
    bool improvedAny = false;
    bool improved = true;
    int n = route.size();

    while (improved)
    {


        improved = false;

        for (int i = 0; i < n - 1; i++)
        {
            for (int j = i + 1; j < n; j++)
            {
                float before = TotalDistCalculator(route, adjacencyMatrixKm);

                reverse(route.begin() + i, route.begin() + j + 1);

                float after = TotalDistCalculator(route, adjacencyMatrixKm);

                if (after < before - 1e-6f)
                {
                    improved = true;
                    improvedAny = true;
                }
                else
                {
                    //desfaz se não melhorou
                    reverse(route.begin() + i, route.begin() + j + 1);
                }
         
            }
        }
    }
    return improvedAny;
}


void VND(vector<int>& route, const vector<vector<float>>& adjacencyMatrixKm) //Variable Neighborhood
{
    bool someImprovement = true;

    while (someImprovement)
    {
        someImprovement = false;

        if (twoOpt(route, adjacencyMatrixKm)) { someImprovement = true; continue; }
        if (swapLocalSearch(route, adjacencyMatrixKm)){ someImprovement = true; continue; }
        if (Shift(route, adjacencyMatrixKm)){ someImprovement = true; continue; }
        if (Inversion(route, adjacencyMatrixKm)){ someImprovement = true; continue; }
    }
}

void printRoute(const vector<int>& route)
{
    for (size_t i = 0; i < route.size(); i++)
    {
        cout << route[i];
        if (i != route.size() - 1) cout << "|";
    }
    cout << endl;
}

int main()
{
    NN nn;

    vector<vector<float>> adjacencyMatrixKm = nn.csv.openCSV("PCV_ Matriz do problema.xlsx - Km.csv");
    vector<vector<float>> adjacencyMatrixMin = nn.csv.openCSV("PCV_ Matriz do problema.xlsx - Min.csv");

    vector<TestCase> tests =
    {
        {"Problema 1 (Km)",  {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47}},
        {"Problema 2 (min)", {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47}},

        {"Problema 3 (Km)",  {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35}},
        {"Problema 4 (min)", {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35}},

        {"Problema 5 (Km)",  {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23}},
        {"Problema 6 (min)", {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23}},

        {"Problema 7 (Km)",  {0,1,2,3,4,5,6,7,8,9,10,11}},
        {"Problema 8 (min)", {0,1,2,3,4,5,6,7,8,9,10,11}},

        {"Problema 9 (Km)",  {0,6,7,8,9,10,11}},
        {"Problema 10 (min)",{0,6,7,8,9,10,11}},

        {"Problema 11 (Km)", {0,1,2,3,4,5}},
        {"Problema 12 (min)",{0,1,2,3,4,5}}
    };

    for(const auto& test : tests)
    {   
        
        int size = test.city.size();
        vector<vector<float>> currentMatrixKm(size, vector<float>(size));
        vector<vector<float>> currentMatrixMin(size, vector<float>(size));

        bool isTime = test.nome.find("min") != string::npos;

        const auto& currentMatrix = isTime ? currentMatrixMin : currentMatrixKm;
        cout << "\n=====================================================\n";
        cout << test.nome << endl;
        cout << "=====================================================\n";






        for(int i = 0; i < size; i++)
        {
            for(int j = 0; j < size; j++)
            {
                currentMatrixKm[i][j] = adjacencyMatrixKm[test.city[i]][test.city[j]];
                currentMatrixMin[i][j] = adjacencyMatrixMin[test.city[i]][test.city[j]];
            }
        }

        vector<int> BestGlobalRoute;
        float BestGlobalDist =numeric_limits<float>::max();

        int BestInicialCity = -1;

        for(int InitialCity = 0; InitialCity < size; InitialCity++)
        {
            vector<int> route = nn.NeighborNearest(InitialCity, size, currentMatrix);

            float PreviousDist = TotalDistCalculator(route, currentMatrix);

            VND(route, currentMatrix);

            float DistAfter = TotalDistCalculator(route, currentMatrix);



            if(DistAfter < BestGlobalDist)
            {
                BestGlobalDist = DistAfter;

                BestGlobalRoute = route;
                BestInicialCity = InitialCity;
            }
        }

        cout << "\nMelhor resultado:\n";
        cout << "Cidade inicial: " << BestInicialCity + 1 << endl;

        cout << "Rota: ";

        for(int index : BestGlobalRoute)
        {
            cout << test.city[index] + 1 << " ";
        }

        cout << test.city[BestGlobalRoute[0]] + 1;

        cout << "\nDistancia: ";

        if (test.nome.find("min") != string::npos)
        {
            int totalMinutes = static_cast<int>(BestGlobalDist);

            cout << totalMinutes << " min " << setw(2) << setfill('0') << endl;
        }
        else
        {

            cout << fixed << setprecision(2) << BestGlobalDist << " Km" << endl;
        }
    }

    return 0;
}