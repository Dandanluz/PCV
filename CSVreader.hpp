#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>

using namespace std;

class CSVREADER
{
    public:
        // Lê uma matriz NxN de um CSV exportado do Excel (formato "PCV_ Matriz do problema.xlsx").
        // Ignora a primeira linha (cabeçalho com os índices das colunas) e a primeira coluna
        // de cada linha (índice da cidade), retornando apenas os valores numéricos da matriz.
        vector<vector<float>> openCSV(const string &path)
        {
            ifstream archive(path);

            if (!archive.is_open())
            {
                cerr << "Falha ao abrir o arquivo: " << path << endl;
                return {};
            }

            string row;
            bool firstrow = true;
            vector<vector<float>> csvArchive;

            while (getline(archive, row))
            {
                if (firstrow)
                {
                    firstrow = false;
                    continue;
                }

                vector<string> area;
                string actualArea;
                bool marks = false;

                for (char c : row)
                {
                    if (c == '\"')
                        marks = !marks;
                    else if (c == ',' && !marks)
                    {
                        area.push_back(actualArea);
                        actualArea.clear();
                    }
                    else
                        actualArea.push_back(c);
                }
                area.push_back(actualArea);

                vector<float> currentRowData;

                // i começa em 1 para pular a primeira coluna (índice da cidade na linha)
                for (size_t i = 1; i < area.size(); i++)
                {
                    string field = area[i];
                    field.erase(remove(field.begin(), field.end(), ' '), field.end());

                    if (field.empty())
                    {
                        // diagonal principal (distância da cidade para ela mesma)
                        currentRowData.push_back(0.0f);
                        continue;
                    }

                    replace(field.begin(), field.end(), ',', '.');

                    try
                    {
                        currentRowData.push_back(stof(field));
                    }
                    catch (...)
                    {
                        currentRowData.push_back(0.0f);
                    }
                }

                csvArchive.push_back(currentRowData);
            }

            archive.close();
            return csvArchive;
        }
};
