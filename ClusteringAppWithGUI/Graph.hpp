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

    // ���ݳ�Ա
    unordered_map<string, Vertex> vertices_;
    vector<string> nodeIds_;

    // ˽�й��߷�������
    vector<string> parse_line(const string& line, char delimiter);

    void find_component_nodes_dfs(const string& current_node, unordered_map<string, bool>& visited, vector<string>& component_nodes) const; // New private helper

public:
    // ���캯������
    explicit Graph(const string& dataPath);

    // �����ӿ�����
    const unordered_map<string, double>& neighbors(const string& id) const;
    void validate_symmetry() const;
    void print_summary() const;

    // ������������
    void perform_neighborhood_analysis(double small_threshold, double medium_threshold,
        double large_threshold, size_t min_neighbors);
    int count_connected_components() const;
    void print_neighborhood_stats() const;
    void export_adjacency_matrix(const string& output_path) const; // ������������
    void export_edge_list_for_cytoscape(const string& output_path, const string& interaction_type) const;
    vector<vector<string>> get_connected_components() const;
    void export_all_components_to_single_file(const string& output_file_path) const;
    void export_all_component_adjacency_matrices_to_single_file(const string& output_file_path) const;
};

/**************** ��Ա����ʵ�� ****************/

// ���캯��ʵ��
Graph::Graph(const string& dataPath) {
    ifstream file(dataPath);
    if (!file) throw runtime_error("�޷����ļ�: " + dataPath);

    // ��ȡ��һ�У��ڵ�ID
    string headerLine;
    if (!getline(file, headerLine)) {
        throw runtime_error("�ļ�Ϊ��");
    }
    nodeIds_ = parse_line(headerLine, '\t');
    if (nodeIds_.empty()) {
        throw runtime_error("�ڵ�ID�б�Ϊ��");
    }

    // ��ʼ��ͼ�ṹ
    for (const auto& id : nodeIds_) {
        vertices_.emplace(id, Vertex());
    }

    // �������������
    for (size_t i = 0; i < nodeIds_.size(); ++i) {
        string dataLine;
        if (!getline(file, dataLine)) {
            throw runtime_error("ȱ�ٵ�" + to_string(i + 1) + "������");
        }

        // ����������
        vector<string> values = parse_line(dataLine, '\t');
        if (values.size() != nodeIds_.size() + 1) {
            throw runtime_error("��" + to_string(i + 1) + "��������ƥ�䣬Ԥ��: " +
                to_string(nodeIds_.size() + 1) + " ʵ��: " + to_string(values.size()));
        }

        // ��֤����ID
        if (values[0] != nodeIds_[i]) {
            throw runtime_error("�ڵ�ID��ƥ��: �� " + to_string(i + 1) +
                " Ԥ�� " + nodeIds_[i] + " ʵ�� " + values[0]);
        }

        // �������ƶ�ֵ
        for (size_t j = 1; j < values.size(); ++j) {
            string targetId = nodeIds_[j - 1];
            if (targetId == nodeIds_[i]) continue; // �����Խ���

            try {
                double weight = stod(values[j]);
                vertices_[nodeIds_[i]].edges[targetId] = weight;
                vertices_[targetId].edges[nodeIds_[i]] = weight;
            }
            catch (const invalid_argument& e) {
                throw runtime_error("��" + to_string(i + 1) + "�е�" +
                    to_string(j + 1) + "��ֵ��Ч: " + values[j]);
            }
        }
    }
}

// �н�������ʵ��
vector<string> Graph::parse_line(const string& line, char delimiter) {
    vector<string> tokens;
    istringstream ss(line);
    string token;
    while (getline(ss, token, delimiter)) {
        // ȥ����β�ո�
        token.erase(remove_if(token.begin(), token.end(), ::isspace), token.end());
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    return tokens;
}

// ��ȡ�ڽӽڵ�ʵ��
const unordered_map<string, double>& Graph::neighbors(const string& id) const {
    return vertices_.at(id).edges;
}

// �Գ�����֤ʵ��
void Graph::validate_symmetry() const {
    for (size_t i = 0; i < nodeIds_.size(); ++i) {
        const string& id1 = nodeIds_[i];
        const auto& edges1 = vertices_.at(id1).edges;
        for (size_t j = 0; j < nodeIds_.size(); ++j) {
            if (i == j) continue;
            const string& id2 = nodeIds_[j];
            const auto& edges2 = vertices_.at(id2).edges;

            if (edges1.find(id2) == edges1.end()) {
                throw runtime_error("����ͼ�Գ����ƻ�: " + id1 + " -> " + id2 + " δ�洢");
            }
            if (abs(edges1.at(id2) - edges2.at(id1)) > 1e-9) {
                throw runtime_error("Ȩ�ز�һ��: " + id1 + " -> " + id2 + " = " +
                    to_string(edges1.at(id2)) + " vs " +
                    to_string(edges2.at(id1)));
            }
        }
    }
    cout << "����ͼ�Գ�����֤ͨ��\n";
}

// ժҪ��ӡʵ��
void Graph::print_summary() const {
    cout << "����ͼ�������\n";
    cout << "�ڵ�����: " << nodeIds_.size() << endl;
}

void Graph::perform_neighborhood_analysis(double small_threshold, double medium_threshold,
    double large_threshold, size_t min_neighbors) {
    cout << "��ʼ�������...\n";
    cout << "��ֵ����: ��=" << small_threshold << ", ��=" << medium_threshold
        << ", С=" << large_threshold << "\n";
    cout << "�ھ�����Լ��: ����=" << min_neighbors << "\n";

    int nodes_processed = 0;
    int small_threshold_cuts = 0;
    int medium_threshold_cuts = 0;
    int large_threshold_cuts = 0;
    int skipped_nodes = 0;

    for (const auto& nodeId : nodeIds_) {
        auto& node = vertices_[nodeId];

        // ����ھ��������ڵ������ޣ�����
        if (node.edges.size() <= min_neighbors) {
            skipped_nodes++;
            continue;
        }

        // ���߰�Ȩ�����򣨴Ӵ�С��
        vector<pair<string, double>> sorted_edges;
        for (const auto& edge : node.edges) {
            sorted_edges.push_back(edge);
        }
        sort(sorted_edges.begin(), sorted_edges.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });

        // ���Ҫɾ���ı�
        vector<string> edges_to_remove;

        // ����С��ֵ�и�
        for (const auto& edge : sorted_edges) {
            if (edge.second < small_threshold) {
                edges_to_remove.push_back(edge.first);
            }
        }

        // ����и��ʣ���ھ�����
        if (node.edges.size() - edges_to_remove.size() >= min_neighbors) {
            // С��ֵ�и���У�ִ��ɾ��
            for (const auto& target : edges_to_remove) {
                // ˫��ɾ�����Ա���ͼ�ĶԳ���
                vertices_[target].edges.erase(nodeId);
                node.edges.erase(target);
            }
            small_threshold_cuts++;
            nodes_processed++;
            continue;
        }

        // С��ֵ�и���У���������ֵ
        edges_to_remove.clear();
        for (const auto& edge : sorted_edges) {
            if (edge.second < medium_threshold) {
                edges_to_remove.push_back(edge.first);
            }
        }

        if (node.edges.size() - edges_to_remove.size() >= min_neighbors) {
            // ����ֵ�и���У�ִ��ɾ��
            for (const auto& target : edges_to_remove) {
                vertices_[target].edges.erase(nodeId);
                node.edges.erase(target);
            }
            medium_threshold_cuts++;
            nodes_processed++;
            continue;
        }

        // ����ֵ�и���У�ֱ��ʹ�ô���ֵ
        edges_to_remove.clear();
        for (const auto& edge : sorted_edges) {
            if (edge.second < large_threshold) {
                edges_to_remove.push_back(edge.first);
            }
        }

        // ���۽����Σ���ִ�д���ֵ�и�
        for (const auto& target : edges_to_remove) {
            vertices_[target].edges.erase(nodeId);
            node.edges.erase(target);
        }
        large_threshold_cuts++;
        nodes_processed++;
    }

    cout << "����������:\n";
    cout << "����ڵ���: " << nodes_processed << "\n";
    cout << "�����ڵ���: " << skipped_nodes << "\n";
    cout << "С��ֵ�и����: " << small_threshold_cuts << "\n";
    cout << "����ֵ�и����: " << medium_threshold_cuts << "\n";
    cout << "����ֵ�и����: " << large_threshold_cuts << "\n";

    int components = count_connected_components();
    cout << "���������ͨ��֧��: " << components << "\n";
}

// ������ͨ��֧����
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

// ������������ʵ�֣���ȡ�������ӵ����
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

// ��ӡ�ھ�ͳ����Ϣ
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

    cout << "�ھ�ͳ����Ϣ:\n";
    cout << "��С�ھ���: " << min_neighbors << "\n";
    cout << "����ھ���: " << max_neighbors << "\n";
    cout << "ƽ���ھ���: " << avg_neighbors << "\n";
}

// ��������ʵ�֣������ڽӾ����ļ�
void Graph::export_adjacency_matrix(const string& output_path) const {
    ofstream outfile(output_path);
    if (!outfile.is_open()) {
        throw runtime_error("�޷�������ļ�: " + output_path);
    }

    // ��ӡ�б��� (�ڵ�����)
    outfile << "\t"; // ���ϽǵĿհ׵�Ԫ��
    for (size_t i = 0; i < nodeIds_.size(); ++i) {
        outfile << nodeIds_[i] << (i == nodeIds_.size() - 1 ? "" : "\t");
    }
    outfile << "\n";

    // ��ӡÿһ������
    for (const string& row_node_id : nodeIds_) {
        outfile << row_node_id << "\t"; // �б��� (�ڵ�����)
        for (size_t i = 0; i < nodeIds_.size(); ++i) {
            const string& col_node_id = nodeIds_[i];
            if (row_node_id == col_node_id) {
                outfile << "-";
            }
            else {
                // ����Ƿ���ڱ�
                // vertices_.at(row_node_id) ȷ���ڵ����
                // .edges.count(col_node_id) ����Ƿ���ڴ� row_node_id �� col_node_id �ı�
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
    cout << "�ڽӾ����ѵ�����: " << output_path << endl;
}

//void Graph::export_components_to_files(const string& output_dir_path) const {
//    vector<vector<string>> components = get_connected_components();
//    cout << "���� " << components.size() << " ����ͨ��֧��" << endl;
//
//    for (size_t i = 0; i < components.size(); ++i) {
//        const auto& component_nodes = components[i];
//        if (component_nodes.empty()) { // Should not happen with the current logic
//            std::cout << "����: ��ͨ��֧ " << (i + 1) << " Ϊ�գ�����������" << std::endl;
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
//            cerr << "����: �޷����ļ� " << file_path_str << " ����д�롣" << endl;
//            continue; // Skip this component if file cannot be opened
//        }
//
//        cout << "���ڵ�����ͨ��֧ " << (i + 1) << " (���� " << component_nodes.size() << " ���ڵ�) ��: " << file_path_str << endl;
//        for (const string& node_id : component_nodes) {
//            outfile << node_id << "\n";
//        }
//        outfile.close();
//    }
//    if (!components.empty()) {
//        cout << "������ͨ��֧�ѵ�����Ŀ¼: " << output_dir_path << endl;
//    } else {
//        cout << "û����ͨ��֧�ɹ�������" << endl;
//    }
//}

void Graph::export_all_components_to_single_file(const string& output_file_path) const {
    vector<vector<string>> components = get_connected_components();

    ofstream outfile(output_file_path);
    if (!outfile.is_open()) {
        throw runtime_error("�޷�������ļ�: " + output_file_path);
    }

    cout << "���� " << components.size() << " ����ͨ��֧�����ڵ����������ļ�: " << output_file_path << endl;

    for (size_t i = 0; i < components.size(); ++i) {
        const auto& component_nodes = components[i];
        if (component_nodes.empty()) {
            outfile << "����: ��ͨ��֧ " << (i + 1) << " Ϊ�ա�\n\n";
            std::cout << "����: ��ͨ��֧ " << (i + 1) << " Ϊ�ա�" << std::endl;
            continue;
        }

        outfile << "Component " << (i + 1) << " (Nodes: " << component_nodes.size() << "):\n";
        for (const string& node_id : component_nodes) {
            outfile << node_id << "\n";
        }
        outfile << "\n"; // Add a blank line between components for better readability
    }

    outfile.close();
    cout << "������ͨ��֧�ѵ�����: " << output_file_path << endl;
}

// �޸ĺ�ĺ���ʵ�֣�����Ϊ Cytoscape �Ѻõı��б��ʽ��������Ȩ��
void Graph::export_edge_list_for_cytoscape(const string& output_path, const string& interaction_type) const {
    ofstream outfile(output_path);
    if (!outfile.is_open()) {
        throw runtime_error("�޷�������ļ�: " + output_path);
    }

    // д���ͷ������ "Weight" ��
    outfile << "SourceNode\tTargetNode\tInteractionType\tWeight\n";

    for (const auto& node_entry : vertices_) {
        const string& source_node = node_entry.first;
        const Vertex& vertex_data = node_entry.second;

        for (const auto& edge_entry : vertex_data.edges) {
            const string& target_node = edge_entry.first;
            double weight = edge_entry.second; // ��ȡ�ߵ�Ȩ��

            // ȷ������ͼ�е�ÿ����ֻ���һ��
            if (source_node < target_node) {
                outfile << source_node << "\t"
                    << target_node << "\t"
                    << interaction_type << "\t"
                    << weight << "\n"; // ��Ȩ��д�������
            }
        }
    }

    outfile.close();
    cout << "Cytoscape ���б� (��Ȩ��) �ѵ�����: " << output_path << endl;
}

void Graph::export_all_component_adjacency_matrices_to_single_file(const string& output_file_path) const {
    // Open the output file
    ofstream outfile(output_file_path);
    if (!outfile.is_open()) {
        throw runtime_error("�޷�������ļ�: " + output_file_path);
    }

    // Get all connected components
    vector<vector<string>> components = get_connected_components();
    cout << "���� " << components.size() << " ����ͨ��֧�����ڵ��������ڽӾ���: " << output_file_path << endl;

    for (size_t i = 0; i < components.size(); ++i) {
        const auto& component_nodes = components[i];
        if (component_nodes.empty()) {
            outfile << "����: ��ͨ��֧ " << (i + 1) << " Ϊ�ա�\n\n";
            cout << "����: ��ͨ��֧ " << (i + 1) << " Ϊ�ա�" << endl;
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
                    // Check if there��s an edge between row_node_id and col_node_id
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
    cout << "������ͨ��֧���ڽӾ����ѵ�����: " << output_file_path << endl;
}


#endif // LSPQ_GRAPH_HPP#pragma once
