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

    Image(size_t w, size_t h) : width(w), height(h) {
        data.resize(height, std::vector<double>(width, 0.0));
    }
};

// Node for Gaussian blur with precision levels
class GaussianBlurNode : public Node<Image> {
public:
    GaussianBlurNode(const Image& input, double sigma, size_t precision_level)
        : Node<Image>("gaussian_blur"), input_(input), sigma_(sigma) {
        tree_ = std::make_unique<FractalTreeNode<Image>>(8, 0.001);
    }

protected:
    Task<Image> compute_impl() override {
        // Check if result exists in fractal tree
        if (auto cached = tree_->get(precision_level_)) {
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

        // Store result in fractal tree
        tree_->store(result, precision_level_);
        
        co_return result;
    }

private:
    Image input_;
    double sigma_;
    std::unique_ptr<FractalTreeNode<Image>> tree_;
    size_t precision_level_ = 4;  // Default precision level
};

// Node for image thresholding
class ThresholdNode : public Node<Image> {
public:
    ThresholdNode(const Image& input, double threshold)
        : Node<Image>("threshold"), input_(input), threshold_(threshold) {}

protected:
    Task<Image> compute_impl() override {
        Image result(input_.width, input_.height);
        
        for (size_t y = 0; y < input_.height; ++y) {
            for (size_t x = 0; x < input_.width; ++x) {
                result.data[y][x] = input_.data[y][x] > threshold_ ? 1.0 : 0.0;
            }
        }

        co_return result;
    }

private:
    Image input_;
    double threshold_;
};

// Generate a test image
Image generate_test_image(size_t width, size_t height) {
    Image img(width, height);
    for (size_t y = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x) {
            double dx = x - width/2;
            double dy = y - height/2;
            double d = std::sqrt(dx*dx + dy*dy);
            img.data[y][x] = std::exp(-d*d / (2 * 50.0 * 50.0));
        }
    }
    return img;
}

int main() {
    const size_t width = 512;
    const size_t height = 512;

    // Create test image
    auto input_image = generate_test_image(width, height);
    std::cout << "Generated test image " << width << "x" << height << "\n";

    // Create graph with thread pool
    auto thread_pool = std::make_shared<ThreadPool>(4);
    Graph<Image> graph(nullptr, thread_pool);

    // Create image processing pipeline
    auto blur = std::make_shared<GaussianBlurNode>(input_image, 2.0, 4);
    auto threshold = std::make_shared<ThresholdNode>(input_image, 0.5);

    // Add nodes to graph
    graph.add_node(blur);
    graph.add_node(threshold);
    graph.add_edge(std::make_shared<Edge<Image>>(blur, threshold));

    // Add callbacks
    blur->add_completion_callback([](const Image& result) {
        std::cout << "Blur operation completed\n";
    });

    threshold->add_completion_callback([](const Image& result) {
        std::cout << "Threshold operation completed\n";
    });

    // Execute graph and measure time
    auto start = std::chrono::high_resolution_clock::now();
    graph.execute().await_resume();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Total execution time: " << duration.count() << "ms\n";

    return 0;
}
