#ifndef JOHNSON_MAXSAT_H
#define JOHNSON_MAXSAT_H

#include <algorithm>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <random>
#include <sstream>
#include <string>
#include <vector>
#include <ctime>
#include <unordered_map>
//#include <unordered_set>
using namespace std;


int sum_soft = 0; //软子句权值总和
//std::vector<double> prob; //每个变量为1的概率

// MaxSatInstance结构体表示MaxSAT问题的实例
struct MaxSatInstance
{
    int num_variables; // 变量的数量
    int num_clauses;  //子句的个数
    std::vector<int> weights;              // 每个子句的权重
    std::vector<std::vector<int>> clauses; // 每个子句的变量
    
    //std::unordered_map<int, std::vector<int>>  position;       // 每个文字所在的子句(从零开始)
    //std::unordered_map<int, std::unordered_set<int>>  neighbor; // 每个文字的邻居（不重复）
};

// 定义一个函数john，输入为变量个数n_vars，子句个数n_clauses，存储所有子句的向量clauses
// 输出为一个向量，存储每个变量的赋值结果
std::vector<int> john(int n_vars, int n_clauses, const std::vector<int> weights, const std::vector<std::vector<int>>& clauses);

void write_to_file(const std::vector<int>& new_assign, const std::string& file_name);

#endif /* MAXSAT_LNS_H */
