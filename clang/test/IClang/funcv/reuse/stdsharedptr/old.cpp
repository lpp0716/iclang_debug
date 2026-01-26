#include <memory>
#include <vector>

class Node {
public:
  int x;
  explicit Node(const int _x) : x(_x) {}
  std::vector<std::weak_ptr<Node>> children;

  void push_back(const std::shared_ptr<Node> &node) {
    children.push_back(node);
  }
};

int test() {
  auto root = std::make_shared<Node>(1);
  auto child = std::make_shared<Node>(2);
  root->push_back(child);
  int sum = 0;
  for (const auto &elem : root->children) {
    sum += elem.lock()->x;
  }
  return sum - root->x - 1;
}

int main() {
  return test();
}