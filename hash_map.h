#ifndef HASH_MAP_H_
#define HASH_MAP_H_

#include <vector>
#include <algorithm>
#include <exception>
#include <iterator>
#include <memory>
#include <iostream>
#include <utility>

namespace hashmap {

static constexpr size_t START_SIZE = 1087;
static constexpr size_t FILL_CONST = 2;
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
    size_t element_count;
    size_t current_size;
    std::vector<std::vector<std::shared_ptr<Element>>> inner_state;
    std::vector<std::shared_ptr<Element>> all_inserted;

    template<typename element_type, typename inner_type>
    class _iterator {
     private:
        inner_type inner;
        inner_type real_end;

     public:
        _iterator() {}

        _iterator(const _iterator& other):
        inner(other.inner),
        real_end(other.real_end) {}

        _iterator(const inner_type& it, const inner_type& end):
        inner(it),
        real_end(end) {}

        _iterator& operator++() {
            ++inner;
            while (inner != real_end && (*inner)->is_marked) {
                ++inner;
            }
            return *this;
        }

        _iterator operator++(int) {
            _iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        element_type& operator*() const {
            return *reinterpret_cast<element_type*>(&(*inner)->val);
        }

        element_type* operator->() const {
            return reinterpret_cast<element_type*>(&(*inner)->val);
        }

        bool operator==(const _iterator& other) const {
            return inner == other.inner;
        }

        bool operator!=(const _iterator &other) const {
            return inner != other.inner;
        }
    };

    explicit HashMap(size_t _size, Hash _hasher = Hash()):
    hasher(_hasher), element_count(0), current_size(_size) {
        inner_state.resize(_size);
    }

    void check_size() {
        if (FILL_CONST * element_count > current_size) {
            migrate();
        }
    }

    void migrate() {
        std::vector<std::shared_ptr<Element>> stored = all_inserted;
        all_inserted.clear();
        inner_state.clear();
        current_size *= FILL_CONST;
        inner_state.resize(current_size);
        element_count = 0;
        for (auto ptr: stored) {
            if (!(ptr->is_marked)) {
                insert(ptr->val);
            }
        }
    }

 public:
    using iterator = _iterator<
    std::pair<const KeyType, ValueType>,
    typename std::vector<std::shared_ptr<Element>>::iterator
    >;
    using const_iterator = _iterator<
    const std::pair<const KeyType, ValueType>,
    typename std::vector<std::shared_ptr<Element>>::const_iterator
    >;

    explicit HashMap(Hash _hasher = Hash()):
    hasher(_hasher), element_count(0), current_size(START_SIZE) {
        inner_state.resize(START_SIZE);
    }

    template <class Iter>
    HashMap(Iter begin, Iter end, Hash _hasher = Hash()):
    hasher(_hasher), element_count(0), current_size(START_SIZE) {
        inner_state.resize(START_SIZE);
        while (begin != end) {
            insert(*begin);
            ++begin;
        }
    }

    HashMap(std::initializer_list<stored_type> list, Hash _hasher = Hash()):
    HashMap(list.begin(), list.end(), _hasher) {}

    HashMap(const HashMap& other) {
        *this = HashMap(other.begin(), other.end());
    }

    HashMap(HashMap&& other) {
        hasher = other.hasher;
        element_count = other.element_count;
        current_size = other.current_size;
        inner_state = std::move(other.inner_state);
        all_inserted = std::move(other.all_inserted);
        return *this;
    }

    HashMap& operator=(const HashMap& other) {
        HashMap tmp(other);
        hasher = tmp.hasher;
        element_count = tmp.element_count;
        current_size = tmp.current_size;
        inner_state = std::move(tmp.inner_state);
        all_inserted = std::move(tmp.all_inserted);
        return *this;
    }

    HashMap& operator=(HashMap&& other) {
        hasher = other.hasher;
        element_count = other.element_count;
        current_size = other.current_size;
        inner_state = std::move(other.inner_state);
        all_inserted = std::move(other.all_inserted);
        return *this;
    }

    size_t size() const {
        return element_count;
    }

    bool empty() const {
        return element_count == 0;
    }

    Hash hash_function() const {
        return hasher;
    }

    void insert(const stored_type &val) {
        size_t hashmap_key = hasher(val.first) % inner_state.size();
        for (const auto &it: inner_state[hashmap_key]) {
            if (it->val.first == val.first) {
                if (it->is_marked) {
                    it->is_marked = false;
                    ++element_count;
                    check_size();
                }
                return;
            }
        }
        std::shared_ptr<Element> to_insert =
        std::make_shared<Element>(val, all_inserted.size());
        inner_state[hashmap_key].push_back(to_insert);
        all_inserted.push_back(to_insert);
        ++element_count;
        check_size();
    }

    void erase(const KeyType &key) {
        size_t hashmap_key = hasher(key) % current_size;

        for (auto &it: inner_state[hashmap_key]) {
            if (it->val.first == key && !it->is_marked) {
                it->is_marked = true;
                --element_count;
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
        size_t hashmap_key = hasher(key) % current_size;

        for (auto &it: inner_state[hashmap_key]) {
            if (it->val.first == key && !it->is_marked) {
                return iterator(all_inserted.begin() + it->pos,
                                all_inserted.end());
            }
        }

        return iterator(all_inserted.end(), all_inserted.end());
    }

    const_iterator find(const KeyType& key) const {
        size_t hashmap_key = hasher(key) % current_size;

        for (auto &it: inner_state[hashmap_key]) {
            if (it->val.first == key && !it->is_marked) {
                return const_iterator(all_inserted.begin() + it->pos,
                                      all_inserted.end());
            }
        }

        return const_iterator(all_inserted.end(), all_inserted.end());
    }

    ValueType& operator[](const KeyType& key) {
        size_t hashmap_key = hasher(key) % current_size;
        for (auto &it: inner_state[hashmap_key]) {
            if (it->val.first == key && !it->is_marked) {
                return it->val.second;
            }
        }

        insert(stored_type(key, ValueType()));
        return all_inserted.back()->val.second;
    }

    const ValueType& at(const KeyType& key) const {
        size_t hashmap_key = hasher(key) % current_size;
        for (auto &it: inner_state[hashmap_key]) {
            if (it->val.first == key && !it->is_marked) {
                return it->val.second;
            }
        }

        throw std::out_of_range("Key not found");
    }

    void clear() {
        all_inserted.clear();
        inner_state.clear();
        element_count = 0;
        current_size = START_SIZE;
        inner_state.resize(START_SIZE);
    }

    void shrink_to_fit() {
        std::vector<std::shared_ptr<Element>> stored;
        for (auto it : all_inserted) {
            if (!it->is_marked) {
                stored.push_back(it);
            }
        }
        all_inserted.clear();
        inner_state.clear();
        current_size = stored.size() * FILL_CONST * FILL_CONST;
        inner_state.resize(current_size);
        element_count = 0;
        for (auto ptr: stored) {
            insert(ptr->val);
        }
    }
};

}  // namespace hashmap

#endif  // HASH_MAP_H_
