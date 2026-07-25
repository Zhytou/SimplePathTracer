#include <mutex>
#include <queue>

namespace spt {

template <typename T>
class Queue {
   public:
    bool pop(T& out) {
        std::lock_guard<std::mutex> lock(m_mtx);
        if (m_queue.empty()) {
            return false;
        }
        out = m_queue.front();
        m_queue.pop();
        return true;
    }
    void push(const T& item) {
        std::lock_guard<std::mutex> lock(m_mtx);
        m_queue.push(item);
    }
    size_t size() {
        std::lock_guard<std::mutex> lock(m_mtx);
        return m_queue.size();
    }

   private:
    std::queue<T> m_queue;
    std::mutex m_mtx;
};

} // namespace spt
