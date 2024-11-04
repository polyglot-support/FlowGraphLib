#include <flowgraph/core/graph.hpp>
#include <flowgraph/core/fractal_tree_node.hpp>
#include <vector>
#include <cmath>
#include <iostream>

using namespace flowgraph;

// Simple image representation
struct Image {
    std::vector<std::vector<double>> data;
    size_t width;
    size_t height;

    // Default constructor required for FractalTreeNode
    Image() : width(0), height(0) {}

    Image(size_t w, size_t h) : width(w), height(h) {
        data.resize(height, std::vector<double>(width, 0.0));
    }

    // Copy constructor for explicit control
    Image(const Image& other) : width(other.width), height(other.height), data(other.data) {}

    // Operator overloads for arithmetic operations
    Image operator*(double scalar) const {
        Image result(width, height);
        for (size_t i = 0; i < height; ++i) {
            for (size_t j = 0; j < width; ++j) {
                result.data[i][j] = data[i][j] * scalar;
            }
        }
        return result;
    }

    Image operator+(const Image& other) const {
        if (width != other.width || height != other.height) {
            throw std::runtime_error("Image dimensions must match for addition");
        }
        Image result(width, height);
        for (size_t i = 0; i < height; ++i) {
            for (size_t j = 0; j < width; ++j) {
                result.data[i][j] = data[i][j] + other.data[i][j];
            }
        }
        return result;
    }

    bool operator==(const Image& other) const {
        return width == other.width && 
               height == other.height && 
               data == other.data;
    }
};

// Add hash specialization for Image type
namespace std {
    template<>
    struct hash<Image> {
        size_t operator()(const Image& img) const {
            size_t seed = hash<size_t>{}(img.width);
            seed ^= hash<size_t>{}(img.height) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            
            for (const auto& row : img.data) {
                for (const auto& val : row) {
                    seed ^= hash<double>{}(val) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
                }
            }
            return seed;
        }
    };
}

// Node for Gaussian blur with precision levels
class GaussianBlurNode : public Node<Image> {
public:
    GaussianBlurNode(const Image& input, double sigma)
        : Node<Image>("gaussian_blur"), input_(input), sigma_(sigma) {
        tree_ = std::make_unique<FractalTreeNode<Image>>(8, 0.001);
    }

protected:
    Task<Image> compute_impl() override {
        // Check cache in fractal tree
        if (auto cached = tree_->get(4)) {
            co_return cached.value();
        }

        // Compute Gaussian kernel
        int kernel_size = static_cast<int>(6 * sigma_);
        if (kernel_size % 2 == 0) kernel_size++;
        std::vector<double> kernel(kernel_size);
        double sum = 0.0;
        int half = kernel_size / 2;

        for (int i = 0; i < kernel_size; ++i) {
            double x = i - half;
            kernel[i] = std::exp(-(x * x) / (2 * sigma_ * sigma_));
            sum += kernel[i];
        }
        for (auto& k : kernel) k /= sum;

        // Apply blur
        Image result(input_.width, input_.height);

        // Horizontal pass
        for (size_t y = 0; y < input_.height; ++y) {
            for (size_t x = 0; x < input_.width; ++x) {
                double sum = 0.0;
                for (int k = -half; k <= half; ++k) {
                    int px = static_cast<int>(x) + k;
                    if (px >= 0 && px < static_cast<int>(input_.width)) {
                        sum += input_.data[y][px] * kernel[k + half];
                    }
                }
                result.data[y][x] = sum;
            }
        }

        // Vertical pass
        Image final_result(input_.width, input_.height);
        for (size_t y = 0; y < input_.height; ++y) {
            for (size_t x = 0; x < input_.width; ++x) {
                double sum = 0.0;
                for (int k = -half; k <= half; ++k) {
                    int py = static_cast<int>(y) + k;
                    if (py >= 0 && py < static_cast<int>(input_.height)) {
                        sum += result.data[py][x] * kernel[k + half];
                    }
                }
                final_result.data[y][x] = sum;
            }
        }

        // Store in fractal tree
        tree_->store(final_result, 4);
        
        co_return final_result;
    }

private:
    Image input_;
    double sigma_;
    std::unique_ptr<FractalTreeNode<Image>> tree_;
};

// Generate a test pattern image
Image generate_test_pattern(size_t width, size_t height) {
    Image img(width, height);
    for (size_t y = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x) {
            // Create a checkerboard pattern
            bool is_white = ((x / 20) + (y / 20)) % 2 == 0;
            img.data[y][x] = is_white ? 1.0 : 0.0;
        }
    }
    return img;
}

// Function to print performance metrics
void print_performance_metrics(const std::chrono::milliseconds& duration, size_t operations) {
    std::cout << "\nPerformance Metrics:\n"
              << "-------------------\n"
              << "Total execution time: " << duration.count() << "ms\n"
              << "Operations performed: " << operations << "\n"
              << "Average time per operation: " << (duration.count() / static_cast<double>(operations)) << "ms\n";
}

int main() {
    const size_t width = 200;
    const size_t height = 200;

    // Create test pattern
    auto input_image = generate_test_pattern(width, height);
    std::cout << "Generated test pattern " << width << "x" << height << "\n";

    // Create graph with thread pool and caching
    auto thread_pool = std::make_shared<ThreadPool>(4);
    Graph<Image> graph(std::make_unique<LRUCachePolicy<Image>>(5), thread_pool);

    // Create Gaussian blur pipeline with different sigma values
    std::vector<std::shared_ptr<GaussianBlurNode>> blur_nodes;
    std::vector<double> sigma_values = {1.0, 2.0, 4.0, 8.0};

    for (double sigma : sigma_values) {
        auto node = std::make_shared<GaussianBlurNode>(input_image, sigma);
        blur_nodes.push_back(node);
        graph.add_node(node);
        
        // Add completion callback
        node->add_completion_callback([sigma](const Image& result) {
            std::cout << "Completed blur with sigma=" << sigma << "\n";
        });
    }

    // Execute graph multiple times to demonstrate caching effects
    std::cout << "\nExecuting graph multiple times to demonstrate caching...\n";
    
    for (int i = 0; i < 3; ++i) {
        std::cout << "\nIteration " << (i + 1) << ":\n";
        auto start = std::chrono::high_resolution_clock::now();
        graph.execute().await_resume();
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        print_performance_metrics(duration, blur_nodes.size());
    }

    return 0;
}
