#include "Johnson_maxsat.h"  // 使用用户提供的头文件
#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <string>
#include <sstream>
#include <cmath>
#include <chrono>
#include <random>
#include <algorithm>  // 添加缺失的头文件

using namespace std;
using namespace std::chrono;

//--------------------- 从Johnson-only.cpp复制的读取函数 ---------------------
MaxSatInstance read_cnf_file(const string &filename) {
    MaxSatInstance instance;
    ifstream cnf_file(filename);
    string line;
    int t = 1;
    sum_soft = 0; // 显式初始化全局变量

    while (getline(cnf_file, line)) {
        if (line.empty() || line[0] == 'c') {
            if (t == 4) {  // 解析变量数（假设特定格式）
                istringstream ss(line);
                string token;
                ss >> token >> token >> instance.num_variables;
            }
            if (t == 5) {  // 解析子句数（假设特定格式）
                istringstream ss(line);
                string token;
                ss >> token >> token >> instance.num_clauses;
            }
            t++;
            continue;
        } 
        
        istringstream iss(line);
        string token;
        iss >> token;

        if (token == "p") {
            continue; // 跳过文件头行
        }
        
        if (token == "h") { // 硬子句
            vector<int> lits;
            int lit;
            while (iss >> lit && lit != 0) {
                lits.push_back(lit);
                instance.num_variables = max(instance.num_variables, abs(lit));
            }
            instance.clauses.push_back(lits);
            instance.weights.push_back(0); // 初始权重设为0
        } else { // 软子句
            // 检查是否为有效数字
            if (!all_of(token.begin(), token.end(), ::isdigit)) {
                cerr << "Error: Invalid weight '" << token << "' in line: " << line << endl;
                exit(1);
            }
            int weight = stoi(token);
            sum_soft += weight;
            vector<int> lits;
            int lit;
            while (iss >> lit && lit != 0) {
                lits.push_back(lit);
                instance.num_variables = max(instance.num_variables, abs(lit));
            }
            instance.clauses.push_back(lits);
            instance.weights.push_back(weight);
        }
    }

    // 硬子句权重设为sum_soft（与Johnson-only.cpp逻辑一致）
    for (size_t i = 0; i < instance.weights.size(); i++) {
        if (instance.weights[i] == 0) {
            instance.weights[i] = sum_soft;
        }
    }

    return instance;
}

//--------------------- 原有逻辑（Clause结构体及后续代码） ---------------------
struct Clause {
    int weight;
    bool is_hard;
    vector<int> literals;
    vector<bool> active_literals;
    bool active;
    int effective_length;

    Clause(int w, bool h, const vector<int>& lits)
        : weight(w), is_hard(h), literals(lits), active(true), effective_length(lits.size()) {
        active_literals.resize(lits.size(), true);
    }

    void removeLiteral(int lit) {
        for (int i = 0; i < literals.size(); ++i) {
            if (active_literals[i] && literals[i] == lit) {
                active_literals[i] = false;
                effective_length--;
            }
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <input.wcnf>" << endl;
        return 1;
    }
    
    clock_t start, end;
    start = clock();

    srand(time(0));

    // 使用Johnson-only.cpp的读取函数
    MaxSatInstance instance = read_cnf_file(argv[1]);
    int max_var = instance.num_variables;

    // 转换为原有Clause结构
    vector<Clause> clauses;
    for (size_t i = 0; i < instance.clauses.size(); i++) {
        bool is_hard = (instance.weights[i] == sum_soft);
        clauses.emplace_back(instance.weights[i], is_hard, instance.clauses[i]);
    }

    unordered_map<int, vector<Clause*>> positive_clauses;
    unordered_map<int, vector<Clause*>> negative_clauses;
    for (Clause& clause : clauses) {
        for (int i = 0; i < clause.literals.size(); ++i) {
            int lit = clause.literals[i];
            int var = abs(lit);
            if (lit > 0) {
                positive_clauses[var].push_back(&clause);
            } else {
                negative_clauses[var].push_back(&clause);
            }
        }
    }

    vector<bool> assigned(max_var + 1, false);
    vector<int> assignment(max_var + 1, 0);
    auto start_time = steady_clock::now();
    const int max_rounds = 50;
    int current_round = 0;
    vector<double> pos_scores(max_var + 1, 0.0);
    vector<double> neg_scores(max_var + 1, 0.0);

    while (current_round < max_rounds) {
        if (duration_cast<seconds>(steady_clock::now() - start_time).count() >= 300)
            break;

        fill(pos_scores.begin(), pos_scores.end(), 0.0);
        fill(neg_scores.begin(), neg_scores.end(), 0.0);

        for (Clause& clause : clauses) {
            if (!clause.active || clause.effective_length == 0) continue;
            double score = clause.weight / pow(2.0, clause.effective_length);
            for (int i = 0; i < clause.literals.size(); ++i) {
                if (!clause.active_literals[i]) continue;
                int lit = clause.literals[i];
                int var = abs(lit);
                if (assigned[var]) continue;
                if (lit > 0) pos_scores[var] += score;
                else neg_scores[var] += score;
            }
        }

        vector<int> vars_to_assign;
        for (int var = 1; var <= max_var; ++var) {
            if (assigned[var]) continue;
            double a = pos_scores[var], b = neg_scores[var];
            double sum = a + b;
            if (sum == 0) continue;
            double p = a / sum;
            if (p > 0.8) {
                assignment[var] = 1;
                vars_to_assign.push_back(var);
                assigned[var] = true;
            } else if (p < 0.2) {
                assignment[var] = 0;
                vars_to_assign.push_back(var);
                assigned[var] = true;
            }
        }

        if (vars_to_assign.empty()) break;

        for (int var : vars_to_assign) {
            int val = assignment[var];
            if (val == 1) {
                for (Clause* cp : positive_clauses[var])
                    cp->active = false;
                for (Clause* cp : negative_clauses[var])
                    cp->removeLiteral(-var);
            } else {
                for (Clause* cp : negative_clauses[var])
                    cp->active = false;
                for (Clause* cp : positive_clauses[var])
                    cp->removeLiteral(var);
            }
        }

        current_round++;
    }

    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<> dis(0.0, 1.0);

    for (int var = 1; var <= max_var; ++var) {
        if (assigned[var]) continue;
        double a = pos_scores[var], b = neg_scores[var];
        double sum = a + b;
        double p = (sum == 0) ? 0.5 : (a / sum);
        assignment[var] = dis(gen) < p ? 1 : 0;
    }

    ofstream out("maxsatoutput.txt");
    for (int var = 1; var <= max_var; ++var)
        out << assignment[var] << endl;
        
    end = clock();
    std::cout << "time = " << double(end - start) / CLOCKS_PER_SEC << "s" << endl;

    return 0;
}