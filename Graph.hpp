/**
 ********************************************
 * @file    :Graph.hpp
 * @author  :XXY
 * @brief   :Graph
 * @date    :2025/9/5
 ********************************************
 */

#ifndef LSPQ_GRAPH_HPP
#define LSPQ_GRAPH_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <stdexcept>
#include <iomanip>
#include <algorithm>
#include <cmath>

using namespace std;

class Graph {
    struct Vertex {
        unordered_map<string, double> edges;
    };

    // 数据成员
    unordered_map<string, Vertex> vertices_;
    vector<string> nodeIds_;

    // 私有工具方法声明
    vector<string> parse_line(const string& line, char delimiter);

    void find_component_nodes_dfs(const string& current_node, unordered_map<string, bool>& visited, vector<string>& component_nodes) const; // New private helper

public:
    // 构造函数声明
    explicit Graph(const string& dataPath);

    // 公共接口声明
    const unordered_map<string, double>& neighbors(const string& id) const;
    void validate_symmetry() const;
    void print_summary() const;

    // 新增公共方法
    void perform_neighborhood_analysis(double small_threshold, double medium_threshold,
        double large_threshold, size_t min_neighbors);
    int count_connected_components() const;
    void print_neighborhood_stats() const;
    void export_adjacency_matrix(const string& output_path) const; // 新增函数声明
    void export_edge_list_for_cytoscape(const string& output_path, const string& interaction_type) const;
    vector<vector<string>> get_connected_components() const;
    void export_all_components_to_single_file(const string& output_file_path) const;
    void export_all_component_adjacency_matrices_to_single_file(const string& output_file_path) const;
};

/**************** 成员函数实现 ****************/

// 构造函数实现
Graph::Graph(const string& dataPath) {
    ifstream file(dataPath);
    if (!file) throw runtime_error("无法打开文件: " + dataPath);

    // 读取第一行：节点ID
    string headerLine;
    if (!getline(file, headerLine)) {
        throw runtime_error("文件为空");
    }
    nodeIds_ = parse_line(headerLine, '\t');
    if (nodeIds_.empty()) {
        throw runtime_error("节点ID列表为空");
    }

    // 初始化图结构
    for (const auto& id : nodeIds_) {
        vertices_.emplace(id, Vertex());
    }

    // 处理后续数据行
    for (size_t i = 0; i < nodeIds_.size(); ++i) {
        string dataLine;
        if (!getline(file, dataLine)) {
            throw runtime_error("缺少第" + to_string(i + 1) + "行数据");
        }

        // 解析数据行
        vector<string> values = parse_line(dataLine, '\t');
        if (values.size() != nodeIds_.size() + 1) {
            throw runtime_error("第" + to_string(i + 1) + "行列数不匹配，预期: " +
                to_string(nodeIds_.size() + 1) + " 实际: " + to_string(values.size()));
        }

        // 验证行首ID
        if (values[0] != nodeIds_[i]) {
            throw runtime_error("节点ID不匹配: 行 " + to_string(i + 1) +
                " 预期 " + nodeIds_[i] + " 实际 " + values[0]);
        }

        // 处理相似度值
        for (size_t j = 1; j < values.size(); ++j) {
            string targetId = nodeIds_[j - 1];
            if (targetId == nodeIds_[i]) continue; // 跳过对角线

            try {
                double weight = stod(values[j]);
                vertices_[nodeIds_[i]].edges[targetId] = weight;
                vertices_[targetId].edges[nodeIds_[i]] = weight;
            }
            catch (const invalid_argument& e) {
                throw runtime_error("第" + to_string(i + 1) + "行第" +
                    to_string(j + 1) + "列值无效: " + values[j]);
            }
        }
    }
}

// 行解析函数实现
vector<string> Graph::parse_line(const string& line, char delimiter) {
    vector<string> tokens;
    istringstream ss(line);
    string token;
    while (getline(ss, token, delimiter)) {
        // 去除首尾空格
        token.erase(remove_if(token.begin(), token.end(), ::isspace), token.end());
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    return tokens;
}

// 获取邻接节点实现
const unordered_map<string, double>& Graph::neighbors(const string& id) const {
    return vertices_.at(id).edges;
}

// 对称性验证实现
void Graph::validate_symmetry() const {
    for (size_t i = 0; i < nodeIds_.size(); ++i) {
        const string& id1 = nodeIds_[i];
        const auto& edges1 = vertices_.at(id1).edges;
        for (size_t j = 0; j < nodeIds_.size(); ++j) {
            if (i == j) continue;
            const string& id2 = nodeIds_[j];
            const auto& edges2 = vertices_.at(id2).edges;

            if (edges1.find(id2) == edges1.end()) {
                throw runtime_error("无向图对称性破坏: " + id1 + " -> " + id2 + " 未存储");
            }
            if (abs(edges1.at(id2) - edges2.at(id1)) > 1e-9) {
                throw runtime_error("权重不一致: " + id1 + " -> " + id2 + " = " +
                    to_string(edges1.at(id2)) + " vs " +
                    to_string(edges2.at(id1)));
            }
        }
    }
    cout << "无向图对称性验证通过\n";
}

// 摘要打印实现
void Graph::print_summary() const {
    cout << "无向图构建完成\n";
    cout << "节点总数: " << nodeIds_.size() << endl;
}

void Graph::perform_neighborhood_analysis(double small_threshold, double medium_threshold,
    double large_threshold, size_t min_neighbors) {
    cout << "开始邻域分析...\n";
    cout << "阈值设置: 大=" << small_threshold << ", 中=" << medium_threshold
        << ", 小=" << large_threshold << "\n";
    cout << "邻居数量约束: 下限=" << min_neighbors << "\n";

    int nodes_processed = 0;
    int small_threshold_cuts = 0;
    int medium_threshold_cuts = 0;
    int large_threshold_cuts = 0;
    int skipped_nodes = 0;

    for (const auto& nodeId : nodeIds_) {
        auto& node = vertices_[nodeId];

        // 如果邻居数量少于等于下限，跳过
        if (node.edges.size() <= min_neighbors) {
            skipped_nodes++;
            continue;
        }

        // 将边按权重排序（从大到小）
        vector<pair<string, double>> sorted_edges;
        for (const auto& edge : node.edges) {
            sorted_edges.push_back(edge);
        }
        sort(sorted_edges.begin(), sorted_edges.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });

        // 标记要删除的边
        vector<string> edges_to_remove;

        // 尝试小阈值切割
        for (const auto& edge : sorted_edges) {
            if (edge.second < small_threshold) {
                edges_to_remove.push_back(edge.first);
            }
        }

        // 检查切割后剩余邻居数量
        if (node.edges.size() - edges_to_remove.size() >= min_neighbors) {
            // 小阈值切割可行，执行删除
            for (const auto& target : edges_to_remove) {
                // 双向删除边以保持图的对称性
                vertices_[target].edges.erase(nodeId);
                node.edges.erase(target);
            }
            small_threshold_cuts++;
            nodes_processed++;
            continue;
        }

        // 小阈值切割不可行，尝试中阈值
        edges_to_remove.clear();
        for (const auto& edge : sorted_edges) {
            if (edge.second < medium_threshold) {
                edges_to_remove.push_back(edge.first);
            }
        }

        if (node.edges.size() - edges_to_remove.size() >= min_neighbors) {
            // 中阈值切割可行，执行删除
            for (const auto& target : edges_to_remove) {
                vertices_[target].edges.erase(nodeId);
                node.edges.erase(target);
            }
            medium_threshold_cuts++;
            nodes_processed++;
            continue;
        }

        // 中阈值切割不可行，直接使用大阈值
        edges_to_remove.clear();
        for (const auto& edge : sorted_edges) {
            if (edge.second < large_threshold) {
                edges_to_remove.push_back(edge.first);
            }
        }

        // 无论结果如何，都执行大阈值切割
        for (const auto& target : edges_to_remove) {
            vertices_[target].edges.erase(nodeId);
            node.edges.erase(target);
        }
        large_threshold_cuts++;
        nodes_processed++;
    }

    cout << "邻域分析完成:\n";
    cout << "处理节点数: " << nodes_processed << "\n";
    cout << "跳过节点数: " << skipped_nodes << "\n";
    cout << "小阈值切割次数: " << small_threshold_cuts << "\n";
    cout << "中阈值切割次数: " << medium_threshold_cuts << "\n";
    cout << "大阈值切割次数: " << large_threshold_cuts << "\n";

    int components = count_connected_components();
    cout << "分析后的连通分支数: " << components << "\n";
}

// 计算连通分支数量
int Graph::count_connected_components() const {
    // The previous implementation used a separate DFS.
    // Now, it leverages the get_connected_components method for consistency and efficiency.
    return get_connected_components().size();
}

void Graph::find_component_nodes_dfs(const string& current_node, unordered_map<string, bool>& visited, vector<string>& component_nodes) const {
    visited[current_node] = true;
    component_nodes.push_back(current_node);

    // Check if current_node actually exists in vertices_ and has edges.
    // It's possible for a node to have no edges after pruning.
    if (vertices_.count(current_node)) {
        for (const auto& edge_pair : vertices_.at(current_node).edges) {
            const string& neighbor_node = edge_pair.first;
            // Ensure neighbor_node is a valid node ID that should be in visited map
            // and has not been visited yet.
            if (visited.count(neighbor_node) && !visited.at(neighbor_node)) {
                find_component_nodes_dfs(neighbor_node, visited, component_nodes);
            }
        }
    }
}

// 新增公共方法实现：获取所有连接的组件
vector<vector<string>> Graph::get_connected_components() const {
    vector<vector<string>> all_components;
    unordered_map<string, bool> visited;

    // Initialize visited map for all known node IDs
    for (const string& node_id : nodeIds_) {
        visited[node_id] = false;
    }

    for (const string& node_id : nodeIds_) {
        // If node_id exists in visited map and has not been visited yet
        if (visited.count(node_id) && !visited.at(node_id)) {
            vector<string> current_component_nodes;
            find_component_nodes_dfs(node_id, visited, current_component_nodes);
            // A component is formed even if it's an isolated node.
            // current_component_nodes will contain at least node_id.
            all_components.push_back(current_component_nodes);
        }
    }
    return all_components;
}

// 打印邻居统计信息
void Graph::print_neighborhood_stats() const {
    size_t min_neighbors = SIZE_MAX;
    size_t max_neighbors = 0;
    double avg_neighbors = 0.0;

    for (const auto& nodeId : nodeIds_) {
        size_t neighbor_count = vertices_.at(nodeId).edges.size();
        min_neighbors = min(min_neighbors, neighbor_count);
        max_neighbors = max(max_neighbors, neighbor_count);
        avg_neighbors += neighbor_count;
    }

    avg_neighbors /= nodeIds_.size();

    cout << "邻居统计信息:\n";
    cout << "最小邻居数: " << min_neighbors << "\n";
    cout << "最大邻居数: " << max_neighbors << "\n";
    cout << "平均邻居数: " << avg_neighbors << "\n";
}

// 新增函数实现：导出邻接矩阵到文件
void Graph::export_adjacency_matrix(const string& output_path) const {
    ofstream outfile(output_path);
    if (!outfile.is_open()) {
        throw runtime_error("无法打开输出文件: " + output_path);
    }

    // 打印列标题 (节点名称)
    outfile << "\t"; // 左上角的空白单元格
    for (size_t i = 0; i < nodeIds_.size(); ++i) {
        outfile << nodeIds_[i] << (i == nodeIds_.size() - 1 ? "" : "\t");
    }
    outfile << "\n";

    // 打印每一行数据
    for (const string& row_node_id : nodeIds_) {
        outfile << row_node_id << "\t"; // 行标题 (节点名称)
        for (size_t i = 0; i < nodeIds_.size(); ++i) {
            const string& col_node_id = nodeIds_[i];
            if (row_node_id == col_node_id) {
                outfile << "-";
            }
            else {
                // 检查是否存在边
                // vertices_.at(row_node_id) 确保节点存在
                // .edges.count(col_node_id) 检查是否存在从 row_node_id 到 col_node_id 的边
                if (vertices_.at(row_node_id).edges.count(col_node_id)) {
                    outfile << "1";
                }
                else {
                    outfile << "0";
                }
            }
            if (i < nodeIds_.size() - 1) {
                outfile << "\t";
            }
        }
        outfile << "\n";
    }

    outfile.close();
    cout << "邻接矩阵已导出到: " << output_path << endl;
}

//void Graph::export_components_to_files(const string& output_dir_path) const {
//    vector<vector<string>> components = get_connected_components();
//    cout << "发现 " << components.size() << " 个连通分支。" << endl;
//
//    for (size_t i = 0; i < components.size(); ++i) {
//        const auto& component_nodes = components[i];
//        if (component_nodes.empty()) { // Should not happen with the current logic
//            std::cout << "警告: 连通分支 " << (i + 1) << " 为空，跳过导出。" << std::endl;
//            continue;
//        }
//
//        string file_path_str = output_dir_path;
//        if (!file_path_str.empty() && file_path_str.back() != '/') {
//            file_path_str += '/';
//        }
//        file_path_str += "component_" + to_string(i + 1) + ".txt";
//
//        ofstream outfile(file_path_str);
//        if (!outfile.is_open()) {
//            cerr << "错误: 无法打开文件 " << file_path_str << " 进行写入。" << endl;
//            continue; // Skip this component if file cannot be opened
//        }
//
//        cout << "正在导出连通分支 " << (i + 1) << " (包含 " << component_nodes.size() << " 个节点) 到: " << file_path_str << endl;
//        for (const string& node_id : component_nodes) {
//            outfile << node_id << "\n";
//        }
//        outfile.close();
//    }
//    if (!components.empty()) {
//        cout << "所有连通分支已导出到目录: " << output_dir_path << endl;
//    } else {
//        cout << "没有连通分支可供导出。" << endl;
//    }
//}

void Graph::export_all_components_to_single_file(const string& output_file_path) const {
    vector<vector<string>> components = get_connected_components();

    ofstream outfile(output_file_path);
    if (!outfile.is_open()) {
        throw runtime_error("无法打开输出文件: " + output_file_path);
    }

    cout << "发现 " << components.size() << " 个连通分支。正在导出到单个文件: " << output_file_path << endl;

    for (size_t i = 0; i < components.size(); ++i) {
        const auto& component_nodes = components[i];
        if (component_nodes.empty()) {
            outfile << "警告: 连通分支 " << (i + 1) << " 为空。\n\n";
            std::cout << "警告: 连通分支 " << (i + 1) << " 为空。" << std::endl;
            continue;
        }

        outfile << "Component " << (i + 1) << " (Nodes: " << component_nodes.size() << "):\n";
        for (const string& node_id : component_nodes) {
            outfile << node_id << "\n";
        }
        outfile << "\n"; // Add a blank line between components for better readability
    }

    outfile.close();
    cout << "所有连通分支已导出到: " << output_file_path << endl;
}

// 修改后的函数实现：导出为 Cytoscape 友好的边列表格式，并包含权重
void Graph::export_edge_list_for_cytoscape(const string& output_path, const string& interaction_type) const {
    ofstream outfile(output_path);
    if (!outfile.is_open()) {
        throw runtime_error("无法打开输出文件: " + output_path);
    }

    // 写入表头，增加 "Weight" 列
    outfile << "SourceNode\tTargetNode\tInteractionType\tWeight\n";

    for (const auto& node_entry : vertices_) {
        const string& source_node = node_entry.first;
        const Vertex& vertex_data = node_entry.second;

        for (const auto& edge_entry : vertex_data.edges) {
            const string& target_node = edge_entry.first;
            double weight = edge_entry.second; // 获取边的权重

            // 确保无向图中的每条边只输出一次
            if (source_node < target_node) {
                outfile << source_node << "\t"
                    << target_node << "\t"
                    << interaction_type << "\t"
                    << weight << "\n"; // 将权重写入第四列
            }
        }
    }

    outfile.close();
    cout << "Cytoscape 边列表 (含权重) 已导出到: " << output_path << endl;
}

void Graph::export_all_component_adjacency_matrices_to_single_file(const string& output_file_path) const {
    // Open the output file
    ofstream outfile(output_file_path);
    if (!outfile.is_open()) {
        throw runtime_error("无法打开输出文件: " + output_file_path);
    }

    // Get all connected components
    vector<vector<string>> components = get_connected_components();
    cout << "发现 " << components.size() << " 个连通分支。正在导出所有邻接矩阵到: " << output_file_path << endl;

    for (size_t i = 0; i < components.size(); ++i) {
        const auto& component_nodes = components[i];
        if (component_nodes.empty()) {
            outfile << "警告: 连通分支 " << (i + 1) << " 为空。\n\n";
            cout << "警告: 连通分支 " << (i + 1) << " 为空。" << endl;
            continue;
        }

        // Write component header
        outfile << "Component " << (i + 1) << " Adjacency Matrix (Nodes: " << component_nodes.size() << "):\n";

        // Write column headers (node IDs in this component)
        outfile << "\t"; // Top-left empty cell
        for (size_t j = 0; j < component_nodes.size(); ++j) {
            outfile << component_nodes[j] << (j == component_nodes.size() - 1 ? "" : "\t");
        }
        outfile << "\n";

        // Write each row of the adjacency matrix
        for (const string& row_node_id : component_nodes) {
            outfile << row_node_id << "\t"; // Row header (node ID)
            for (size_t j = 0; j < component_nodes.size(); ++j) {
                const string& col_node_id = component_nodes[j];
                if (row_node_id == col_node_id) {
                    outfile << "-"; // Diagonal
                }
                else {
                    // Check if there’s an edge between row_node_id and col_node_id
                    if (vertices_.at(row_node_id).edges.count(col_node_id)) {
                        outfile << "1"; // Edge exists
                    }
                    else {
                        outfile << "0"; // No edge
                    }
                }
                if (j < component_nodes.size() - 1) {
                    outfile << "\t";
                }
            }
            outfile << "\n";
        }
        outfile << "\n"; // Add a blank line after each matrix for readability
    }

    outfile.close();
    cout << "所有连通分支的邻接矩阵已导出到: " << output_file_path << endl;
}


#endif // LSPQ_GRAPH_HPP#pragma once
