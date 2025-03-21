#include <vector>
#include <utility>  // For std::pair

// PriorityQueue class template
template <typename T>
class PriorityQueue {
 private:
  std::vector<std::pair<T, float>> elements;

 public:
  // Get the number of elements in the queue
  size_t count() const { return elements.size(); }

  // Enqueue an element with a priority
  void enqueue(const T& item, float priority) {
    elements.emplace_back(item, priority);
  }

  // Dequeue the element with the highest priority (lowest priority value)
  T dequeue() {
    if (elements.empty()) {
      throw std::runtime_error("PriorityQueue is empty");
    }

    // Find the element with the highest priority (smallest priority value)
    auto bestIndex = 0;
    for (size_t i = 1; i < elements.size(); ++i) {
      if (elements[i].second < elements[bestIndex].second) {
        bestIndex = i;
      }
    }

    T bestItem = elements[bestIndex].first;
    elements.erase(elements.begin() + bestIndex);
    return bestItem;
  }
};