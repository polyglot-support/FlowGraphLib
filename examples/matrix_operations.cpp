#include <flowgraph/core/graph.hpp>
#include <vector>
#include <random>
#include <chrono>
#include <iostream>
#include <functional>

using namespace flowgraph;
using Matrix = std::vector<std::vector<double>>;

// Add hash specialization for Matrix type
namespace std {
    template<>
    struct hash<Matrix> {
        size_t operator()(const Matrix& m) const {
            size_t seed = m.size();
            for (const auto& row : m) {
                seed ^= row.size() + 0x9e3779b9 + (seed << 6) + (seed >> 2);
                for (const auto& val : row) {
                    seed ^= std::hash<double>{}(val) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
                }
            }
            return seed;
        }
    };

    // Add equality operator for Matrix type
    template<>
    struct equal_to<Matrix> {
        bool operator()(const Matrix& lhs, const Matrix& rhs) const {
            if (lhs.size() != rhs.size()) return false;
            for (size_t i = 0; i < lhs.size(); ++i) {
                if (lhs[i].size() != rhs[i].size()) return false;
                for (size_t j = 0; j < lhs[i].size(); ++j) {
                    if (lhs[i][j] != rhs[i][j]) return false;
                }
            }
            return true;
        }
    };
}

// Node for matrix multiplication
class MatrixMultiplyNode : public Node<Matrix> {
public:
    MatrixMultiplyNode(const Matrix& a, const Matrix& b) 
        : Node<Matrix>("matrix_multiply"), a_(a), b_(b) {}

protected:
    Task<Matrix> compute_impl() override {
        if (a_.empty() || b_.empty() || a_[0].size() != b_.size()) {
            throw std::runtime_error("Invalid matrix dimensions");
        }

        size_t rows = a_.size();
        size_t cols = b_[0].size();
        size_t inner = b_.size();

        Matrix result(rows, std::vector<double>(cols, 0.0));

        for (size_t i = 0; i < rows; ++i) {
            for (size_t j = 0; j < cols; ++j) {
                for (size_t k = 0; k < inner; ++k) {
                    result[i][j] += a_[i][k] * b_[k][j];
                }
            }
        }

        co_return result;
    }

private:
    Matrix a_, b_;
};

// Node for matrix addition
class MatrixAddNode : public Node<Matrix> {
public:
    MatrixAddNode(const Matrix& a, const Matrix& b) 
        : Node<Matrix>("matrix_add"), a_(a), b_(b) {}

protected:
    Task<Matrix> compute_impl() override {
        if (a_.size() != b_.size() || a_[0].size() != b_[0].size()) {
            throw std::runtime_error("Matrix dimensions must match");
        }

        size_t rows = a_.size();
        size_t cols = a_[0].size();
        Matrix result(rows, std::vector<double>(cols));

        for (size_t i = 0; i < rows; ++i) {
            for (size_t j = 0; j < cols; ++j) {
                result[i][j] = a_[i][j] + b_[i][j];
            }
        }

        co_return result;
    }

private:
    Matrix a_, b_;
};

// Utility functions
Matrix generate_random_matrix(size_t rows, size_t cols) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(-1.0, 1.0);

    Matrix matrix(rows, std::vector<double>(cols));
    for (auto& row : matrix) {
        for (auto& elem : row) {
            elem = dis(gen);
        }
    }
    return matrix;
}

void print_matrix_info(const Matrix& m, const std::string& name) {
    std::cout << name << " dimensions: " << m.size() << "x" << m[0].size() << "\n";
}

int main() {
    // Create matrices
    const size_t size = 100;
    auto A = generate_random_matrix(size, size);
    auto B = generate_random_matrix(size, size);
    auto C = generate_random_matrix(size, size);

    print_matrix_info(A, "Matrix A");
    print_matrix_info(B, "Matrix B");
    print_matrix_info(C, "Matrix C");

    // Create thread pool with different configurations
    std::cout << "\nTesting with different thread pool configurations:\n";

    std::vector<size_t> thread_counts = {1, 2, 4, 8};
    for (auto thread_count : thread_counts) {
        auto thread_pool = std::make_shared<ThreadPool>(thread_count);
        Graph<Matrix> graph(std::make_unique<LRUCachePolicy<Matrix>>(5), thread_pool);

        // Create computation nodes
        auto mult1 = std::make_shared<MatrixMultiplyNode>(A, B);
        auto mult2 = std::make_shared<MatrixMultiplyNode>(B, C);
        auto add = std::make_shared<MatrixAddNode>(A, C);

        // Add nodes to graph
        graph.add_node(mult1);
        graph.add_node(mult2);
        graph.add_node(add);

        // Add completion callbacks
        mult1->add_completion_callback([](const Matrix& result) {
            print_matrix_info(result, "A*B");
        });

        mult2->add_completion_callback([](const Matrix& result) {
            print_matrix_info(result, "B*C");
        });

        add->add_completion_callback([](const Matrix& result) {
            print_matrix_info(result, "A+C");
        });

        // Measure execution time
        auto start = std::chrono::high_resolution_clock::now();
        graph.execute().await_resume();
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        std::cout << "Execution time with " << thread_count 
                  << " threads: " << duration.count() << "ms\n";
    }

    return 0;
}
