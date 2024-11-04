#include <flowgraph/core/graph.hpp>
#include <flowgraph/core/fractal_tree_node.hpp>
#include <vector>
#include <cmath>
#include <random>
#include <iostream>
#include <chrono>

using namespace flowgraph;
using Signal = std::vector<double>;

// Node for generating a sine wave
class SineWaveNode : public Node<Signal> {
public:
    SineWaveNode(double frequency, double amplitude, size_t samples)
        : Node<Signal>("sine_wave"), 
          frequency_(frequency), 
          amplitude_(amplitude),
          samples_(samples) {}

protected:
    Task<Signal> compute_impl() override {
        Signal signal(samples_);
        for (size_t i = 0; i < samples_; ++i) {
            double t = static_cast<double>(i) / samples_;
            signal[i] = amplitude_ * std::sin(2 * M_PI * frequency_ * t);
        }
        co_return signal;
    }

private:
    double frequency_;
    double amplitude_;
    size_t samples_;
};

// Node for adding noise to a signal
class NoiseNode : public Node<Signal> {
public:
    NoiseNode(const Signal& input, double noise_level)
        : Node<Signal>("noise"), input_(input), noise_level_(noise_level) {}

protected:
    Task<Signal> compute_impl() override {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::normal_distribution<> d(0.0, noise_level_);

        Signal noisy = input_;
        for (auto& sample : noisy) {
            sample += d(gen);
        }
        co_return noisy;
    }

private:
    Signal input_;
    double noise_level_;
};

// Node for applying a moving average filter
class MovingAverageNode : public Node<Signal> {
public:
    MovingAverageNode(const Signal& input, size_t window_size)
        : Node<Signal>("moving_average"), 
          input_(input),
          window_size_(window_size) {
        tree_ = std::make_unique<FractalTreeNode<Signal>>(8, 0.001);
    }

protected:
    Task<Signal> compute_impl() override {
        // Check cache
        if (auto cached = tree_->get(4)) {
            co_return cached.value();
        }

        Signal filtered(input_.size());
        for (size_t i = 0; i < input_.size(); ++i) {
            double sum = 0.0;
            size_t count = 0;
            for (size_t j = 0; j < window_size_; ++j) {
                if (i >= j) {
                    sum += input_[i - j];
                    count++;
                }
            }
            filtered[i] = sum / count;
        }

        // Store in cache
        tree_->store(filtered, 4);
        co_return filtered;
    }

private:
    Signal input_;
    size_t window_size_;
    std::unique_ptr<FractalTreeNode<Signal>> tree_;
};

int main() {
    const size_t num_samples = 1000;
    
    // Create thread pool
    auto thread_pool = std::make_shared<ThreadPool>(4);
    Graph<Signal> graph(std::make_unique<LRUCachePolicy<Signal>>(10), thread_pool);

    // Create signal processing pipeline
    auto sine_wave = std::make_shared<SineWaveNode>(10.0, 1.0, num_samples);
    auto noisy_signal = std::make_shared<NoiseNode>(Signal(num_samples), 0.2);
    auto filtered_signal = std::make_shared<MovingAverageNode>(Signal(num_samples), 5);

    // Add nodes and edges
    graph.add_node(sine_wave);
    graph.add_node(noisy_signal);
    graph.add_node(filtered_signal);
    
    graph.add_edge(std::make_shared<Edge<Signal>>(sine_wave, noisy_signal));
    graph.add_edge(std::make_shared<Edge<Signal>>(noisy_signal, filtered_signal));

    // Add callbacks for monitoring
    sine_wave->add_completion_callback([](const Signal& signal) {
        std::cout << "Sine wave generated: " << signal.size() << " samples\n";
    });

    filtered_signal->add_completion_callback([](const Signal& signal) {
        std::cout << "Signal filtered: " << signal.size() << " samples\n";
        
        // Calculate and print signal statistics
        double sum = 0.0, sq_sum = 0.0;
        for (const auto& sample : signal) {
            sum += sample;
            sq_sum += sample * sample;
        }
        double mean = sum / signal.size();
        double rms = std::sqrt(sq_sum / signal.size());
        
        std::cout << "Signal statistics:\n"
                  << "  Mean: " << mean << "\n"
                  << "  RMS: " << rms << "\n";
    });

    // Execute graph and measure performance
    auto start = std::chrono::high_resolution_clock::now();
    graph.execute().await_resume();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Total execution time: " << duration.count() << "ms\n";

    return 0;
}
