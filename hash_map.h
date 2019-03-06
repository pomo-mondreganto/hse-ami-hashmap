#include <vector>
#include <algorithm>
#include <exception>
#include <iterator>
#include <memory>
#include <iostream>
#include <utility>

const size_t TABLE_SIZE = 277051;
template <class KeyType, class ValueType, class Hash = std::hash<KeyType>>
class HashMap {
 private:
    using stored_type = std::pair<KeyType, ValueType>;

    struct Element {
        stored_type val;
        bool is_marked;
        size_t pos;

        Element(stored_type _val, size_t _pos):
        val(_val), is_marked(false), pos(_pos) {}

        Element() {}
    };

    Hash hasher;
    size_t inner_size;
    std::vector<std::vector<std::shared_ptr<Element>>> inner_state;
    std::vector<std::shared_ptr<Element>> all_inserted;

 public:
    class iterator {
     private:
        using inner_type = typename std::vector<
        std::shared_ptr<Element>
        >::iterator;
        inner_type inner;
        inner_type real_end;

     public:
        iterator() {}

        iterator(const iterator& other):
        inner(other.inner),
        real_end(other.real_end) {}

        iterator(const inner_type& it, const inner_type& end):
        inner(it),
        real_end(end) {}

        iterator& operator++() {
            ++inner;
            while (inner != real_end && (*inner)->is_marked) {
                ++inner;
            }
            return *this;
        }

        iterator operator++(int) {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        std::pair<const KeyType, ValueType>& operator*() const {
            return *reinterpret_cast<
            std::pair<const KeyType, ValueType>*
            >(&(*inner)->val);
        }

        std::pair<const KeyType, ValueType>* operator->() const {
            return reinterpret_cast<
            std::pair<const KeyType, ValueType>*
            >(&(*inner)->val);
        }

        bool operator==(const iterator& other) const {
            return inner == other.inner;
        }

        bool operator!=(const iterator &other) const {
            return inner != other.inner;
        }
    };

    class const_iterator {
     private:
        using inner_type = typename std::vector<
        std::shared_ptr<Element>
        >::const_iterator;
        inner_type inner;
        inner_type real_end;

     public:
        const_iterator() {}

        const_iterator(const const_iterator& other):
        inner(other.inner),
        real_end(other.real_end) {}

        const_iterator(const inner_type& it, const inner_type& end):
        inner(it),
        real_end(end) {}

        const_iterator& operator++() {
            ++inner;
            while (inner != real_end && (*inner)->is_marked) {
                ++inner;
            }
            return *this;
        }

        const_iterator operator++(int) {
            const_iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        const std::pair<const KeyType, ValueType>& operator*() const {
            return *reinterpret_cast<
            const std::pair<const KeyType, ValueType>*
            >(&(*inner)->val);
        }

        const std::pair<const KeyType, ValueType>* operator->() const {
            return reinterpret_cast<
            const std::pair<const KeyType, ValueType>*
            >(&(*inner)->val);
        }

        bool operator==(const const_iterator& other) const {
            return inner == other.inner;
        }

        bool operator!=(const const_iterator &other) const {
            return inner != other.inner;
        }
    };

    explicit HashMap(Hash _hasher = Hash()):
    hasher(_hasher), inner_size(0) {
        inner_state.resize(TABLE_SIZE);
    }

    template <class Iter>
    HashMap(Iter begin, Iter end, Hash _hasher = Hash()):
    hasher(_hasher), inner_size(0) {
        inner_state.resize(TABLE_SIZE);
        while (begin != end) {
            insert(*begin);
            ++begin;
        }
    }

    HashMap(std::initializer_list<stored_type> list, Hash _hasher = Hash()):
    hasher(_hasher), inner_size(0) {
        inner_state.resize(TABLE_SIZE);
        for (auto it: list) {
            insert(it);
        }
    }

    size_t size() const {
        return inner_size;
    }

    bool empty() const {
        return inner_size == 0;
    }

    Hash hash_function() const {
        return hasher;
    }

    void insert(const stored_type &val) {
        size_t hashmap_key = hasher(val.first) % inner_state.size();
        for (const auto &it: inner_state[hashmap_key]) {
            if (it->val.first == val.first && !it->is_marked) {
                return;
            }
        }
        std::shared_ptr<Element> to_insert =
        std::make_shared<Element>(val, all_inserted.size());
        inner_state[hashmap_key].push_back(to_insert);
        all_inserted.push_back(to_insert);
        ++inner_size;
    }

    void erase(const KeyType &key) {
        size_t hashmap_key = hasher(key) % inner_state.size();

        for (auto &it: inner_state[hashmap_key]) {
            if (it->val.first == key && !it->is_marked) {
                it->is_marked = true;
                --inner_size;
                return;
            }
        }
    }

    iterator begin() {
        auto tmp = all_inserted.begin();
        while (tmp != all_inserted.end() && (*tmp)->is_marked)
            ++tmp;
        return iterator(tmp, all_inserted.end());
    }

    const_iterator begin() const {
        auto tmp = all_inserted.begin();
        while (tmp != all_inserted.end() && (*tmp)->is_marked)
            ++tmp;
        return const_iterator(tmp, all_inserted.end());
    }

    iterator end() {
        return iterator(all_inserted.end(), all_inserted.end());
    }

    const_iterator end() const {
        return const_iterator(all_inserted.end(), all_inserted.end());
    }

    iterator find(const KeyType& key) {
        size_t hashmap_key = hasher(key) % inner_state.size();

        for (auto &it: inner_state[hashmap_key]) {
            if (it->val.first == key && !it->is_marked) {
                return iterator(all_inserted.begin() + it->pos,
                                all_inserted.end());
            }
        }

        return iterator(all_inserted.end(), all_inserted.end());
    }

    const_iterator find(const KeyType& key) const {
        size_t hashmap_key = hasher(key) % inner_state.size();

        for (auto &it: inner_state[hashmap_key]) {
            if (it->val.first == key && !it->is_marked) {
                return const_iterator(all_inserted.begin() + it->pos,
                                      all_inserted.end());
            }
        }

        return const_iterator(all_inserted.end(), all_inserted.end());
    }

    ValueType& operator[](const KeyType& key) {
        size_t hashmap_key = hasher(key) % inner_state.size();
        for (auto &it: inner_state[hashmap_key]) {
            if (it->val.first == key && !it->is_marked) {
                return it->val.second;
            }
        }

        insert(stored_type(key, ValueType()));
        return all_inserted.back()->val.second;
    }

    const ValueType& at(const KeyType& key) const {
        size_t hashmap_key = hasher(key) % inner_state.size();
        for (auto &it: inner_state[hashmap_key]) {
            if (it->val.first == key && !it->is_marked) {
                return it->val.second;
            }
        }

        throw std::out_of_range("Key not found");
    }

    void clear() {
        for (auto& it: all_inserted) {
            size_t hashmap_key = hasher(it->val.first) % inner_state.size();
            inner_state[hashmap_key].clear();
        }
        all_inserted.clear();
        inner_size = 0;
    }
};
