#include <flowgraph/core/graph.hpp>
#include <flowgraph/core/fractal_tree_node.hpp>
#include <vector>
#include <random>
#include <cmath>
#include <iostream>

using namespace flowgraph;

// Neural network types
using Vector = std::vector<double>;
using Matrix = std::vector<Vector>;

// Forward declarations
class NeuralNetNode;
class DenseLayer;
class ActivationLayer;

// Neural network base node
class NeuralNetNode : public Node<Vector> {
public:
    NeuralNetNode(const std::string& name) : Node<Vector>(name) {}
    
    virtual void backward(const Vector& gradient) = 0;
    virtual void update_weights(double learning_rate) = 0;
};

// Dense layer node with weight management using FractalTreeNode
class DenseLayer : public NeuralNetNode {
public:
    DenseLayer(size_t input_size, size_t output_size, const std::string& name)
        : NeuralNetNode(name), 
          input_size_(input_size), 
          output_size_(output_size),
          weights_tree_(std::make_unique<FractalTreeNode<Matrix>>(8, 0.001)),
          bias_tree_(std::make_unique<FractalTreeNode<Vector>>(8, 0.001)) {
        
        // Initialize weights and biases
        std::random_device rd;
        std::mt19937 gen(rd());
        std::normal_distribution<> d(0.0, std::sqrt(2.0 / input_size));
        
        Matrix weights(output_size, Vector(input_size));
        Vector bias(output_size, 0.0);
        
        for (auto& row : weights) {
            for (auto& w : row) {
                w = d(gen);
            }
        }
        
        weights_tree_->store(weights, 4);
        bias_tree_->store(bias, 4);
    }

protected:
    Task<Vector> compute_impl() override {
        auto weights_opt = weights_tree_->get(4);
        auto bias_opt = bias_tree_->get(4);
        
        if (!weights_opt || !bias_opt) {
            throw std::runtime_error("Weights or bias not initialized");
        }
        
        const auto& weights = weights_opt.value();
        const auto& bias = bias_opt.value();
        
        Vector output(output_size_);
        for (size_t i = 0; i < output_size_; ++i) {
            output[i] = bias[i];
            for (size_t j = 0; j < input_size_; ++j) {
                output[i] += weights[i][j] * last_input_[j];
            }
        }
        
        last_output_ = output;
        co_return output;
    }
    
    void backward(const Vector& gradient) override {
        auto weights_opt = weights_tree_->get(4);
        if (!weights_opt) {
            throw std::runtime_error("Weights not initialized");
        }
        
        const auto& weights = weights_opt.value();
        weight_gradients_.resize(output_size_, Vector(input_size_));
        bias_gradients_.resize(output_size_);
        
        // Compute gradients
        for (size_t i = 0; i < output_size_; ++i) {
            bias_gradients_[i] = gradient[i];
            for (size_t j = 0; j < input_size_; ++j) {
                weight_gradients_[i][j] = gradient[i] * last_input_[j];
            }
        }
    }
    
    void update_weights(double learning_rate) override {
        auto weights_opt = weights_tree_->get(4);
        auto bias_opt = bias_tree_->get(4);
        
        if (!weights_opt || !bias_opt) {
            throw std::runtime_error("Weights or bias not initialized");
        }
        
        auto weights = weights_opt.value();
        auto bias = bias_opt.value();
        
        // Update weights and biases
        for (size_t i = 0; i < output_size_; ++i) {
            bias[i] -= learning_rate * bias_gradients_[i];
            for (size_t j = 0; j < input_size_; ++j) {
                weights[i][j] -= learning_rate * weight_gradients_[i][j];
            }
        }
        
        weights_tree_->store(weights, 4);
        bias_tree_->store(bias, 4);
    }

private:
    size_t input_size_;
    size_t output_size_;
    std::unique_ptr<FractalTreeNode<Matrix>> weights_tree_;
    std::unique_ptr<FractalTreeNode<Vector>> bias_tree_;
    Vector last_input_;
    Vector last_output_;
    Matrix weight_gradients_;
    Vector bias_gradients_;
};

// Activation layer node
class ActivationLayer : public NeuralNetNode {
public:
    enum class ActivationType {
        ReLU,
        Sigmoid,
        Tanh
    };
    
    ActivationLayer(ActivationType type, const std::string& name)
        : NeuralNetNode(name), type_(type) {}

protected:
    Task<Vector> compute_impl() override {
        Vector output(last_input_.size());
        
        switch (type_) {
            case ActivationType::ReLU:
                for (size_t i = 0; i < output.size(); ++i) {
                    output[i] = std::max(0.0, last_input_[i]);
                }
                break;
            case ActivationType::Sigmoid:
                for (size_t i = 0; i < output.size(); ++i) {
                    output[i] = 1.0 / (1.0 + std::exp(-last_input_[i]));
                }
                break;
            case ActivationType::Tanh:
                for (size_t i = 0; i < output.size(); ++i) {
                    output[i] = std::tanh(last_input_[i]);
                }
                break;
        }
        
        last_output_ = output;
        co_return output;
    }
    
    void backward(const Vector& gradient) override {
        input_gradient_.resize(last_input_.size());
        
        switch (type_) {
            case ActivationType::ReLU:
                for (size_t i = 0; i < last_input_.size(); ++i) {
                    input_gradient_[i] = last_input_[i] > 0 ? gradient[i] : 0;
                }
                break;
            case ActivationType::Sigmoid:
                for (size_t i = 0; i < last_input_.size(); ++i) {
                    double s = 1.0 / (1.0 + std::exp(-last_input_[i]));
                    input_gradient_[i] = gradient[i] * s * (1 - s);
                }
                break;
            case ActivationType::Tanh:
                for (size_t i = 0; i < last_input_.size(); ++i) {
                    double t = std::tanh(last_input_[i]);
                    input_gradient_[i] = gradient[i] * (1 - t * t);
                }
                break;
        }
    }
    
    void update_weights(double learning_rate) override {
        // Activation layers have no weights to update
    }

private:
    ActivationType type_;
    Vector last_input_;
    Vector last_output_;
    Vector input_gradient_;
};

// Neural Network Graph wrapper
class NeuralNetwork {
public:
    NeuralNetwork() : graph_(nullptr, std::make_shared<ThreadPool>(4)) {}
    
    void add_layer(std::shared_ptr<NeuralNetNode> layer) {
        if (!layers_.empty()) {
            graph_.add_edge(std::make_shared<Edge<Vector>>(layers_.back(), layer));
        }
        layers_.push_back(layer);
        graph_.add_node(layer);
    }
    
    Vector forward(const Vector& input) {
        // Set input to first layer
        if (layers_.empty()) return input;
        
        // Execute the graph
        graph_.execute().await_resume();
        
        // Return the output of the last layer
        return Vector(); // TODO: Implement
    }
    
    void train(const std::vector<Vector>& inputs, const std::vector<Vector>& targets,
              double learning_rate, size_t epochs, size_t batch_size) {
        for (size_t epoch = 0; epoch < epochs; ++epoch) {
            double total_loss = 0.0;
            
            for (size_t i = 0; i < inputs.size(); i += batch_size) {
                size_t current_batch_size = std::min(batch_size, inputs.size() - i);
                
                // Forward pass
                Vector output = forward(inputs[i]);
                
                // Compute loss and gradients
                Vector loss_gradient(output.size());
                for (size_t j = 0; j < output.size(); ++j) {
                    loss_gradient[j] = output[j] - targets[i][j];
                    total_loss += loss_gradient[j] * loss_gradient[j];
                }
                
                // Backward pass
                for (auto it = layers_.rbegin(); it != layers_.rend(); ++it) {
                    (*it)->backward(loss_gradient);
                }
                
                // Update weights
                for (auto& layer : layers_) {
                    layer->update_weights(learning_rate);
                }
            }
            
            // Print progress
            if (epoch % 10 == 0) {
                std::cout << "Epoch " << epoch << ", Loss: " << total_loss / inputs.size() << std::endl;
            }
        }
    }

private:
    Graph<Vector> graph_;
    std::vector<std::shared_ptr<NeuralNetNode>> layers_;
};

// Example usage
int main() {
    // Create a simple neural network for XOR problem
    NeuralNetwork nn;
    
    // Add layers
    nn.add_layer(std::make_shared<DenseLayer>(2, 4, "hidden1"));
    nn.add_layer(std::make_shared<ActivationLayer>(ActivationLayer::ActivationType::ReLU, "relu1"));
    nn.add_layer(std::make_shared<DenseLayer>(4, 1, "output"));
    nn.add_layer(std::make_shared<ActivationLayer>(ActivationLayer::ActivationType::Sigmoid, "sigmoid1"));
    
    // Training data for XOR
    std::vector<Vector> inputs = {
        {0, 0}, {0, 1}, {1, 0}, {1, 1}
    };
    std::vector<Vector> targets = {
        {0}, {1}, {1}, {0}
    };
    
    // Train the network
    std::cout << "Training XOR network...\n";
    nn.train(inputs, targets, 0.1, 1000, 4);
    
    // Test the network
    std::cout << "\nTesting XOR network:\n";
    for (size_t i = 0; i < inputs.size(); ++i) {
        Vector output = nn.forward(inputs[i]);
        std::cout << inputs[i][0] << " XOR " << inputs[i][1] 
                  << " = " << output[0] << std::endl;
    }
    
    return 0;
}
