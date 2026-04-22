/**
* implement a container like std::map
*/
#ifndef SJTU_MAP_HPP
#define SJTU_MAP_HPP

#include <functional>
#include <cstddef>
#include "utility.hpp"
#include "exceptions.hpp"

namespace sjtu {

template<
   class Key,
   class T,
   class Compare = std::less<Key>
> class map {
  public:
   typedef pair<const Key, T> value_type;

  private:
   struct Node {
     value_type kv;
     unsigned long long pri;
     Node *l, *r, *p;
     Node(const value_type &v, unsigned long long pr, Node *par) : kv(v), pri(pr), l(nullptr), r(nullptr), p(par) {}
   };

   Node *root = nullptr;
   size_t sz = 0;
   Compare comp;

   static unsigned long long rng_seed() {
     static unsigned long long s = 1469598103934665603ull; // FNV offset
     // simple LCG
     s = s * 11400714819323198485ull + 14029467366897019727ull;
     return s;
   }

   bool eq_key(const Key &a, const Key &b) const { return !comp(a, b) && !comp(b, a); }

   Node *leftmost(Node *x) const { while (x && x->l) x = x->l; return x; }
   Node *rightmost(Node *x) const { while (x && x->r) x = x->r; return x; }

   void rotate_left(Node *x) {
     Node *y = x->r; if (!y) return;
     x->r = y->l; if (y->l) y->l->p = x;
     y->p = x->p;
     if (!x->p) root = y; else if (x->p->l == x) x->p->l = y; else x->p->r = y;
     y->l = x; x->p = y;
   }
   void rotate_right(Node *x) {
     Node *y = x->l; if (!y) return;
     x->l = y->r; if (y->r) y->r->p = x;
     y->p = x->p;
     if (!x->p) root = y; else if (x->p->l == x) x->p->l = y; else x->p->r = y;
     y->r = x; x->p = y;
   }

   Node *bst_find(const Key &key) const {
     Node *cur = root;
     while (cur) {
       if (eq_key(key, cur->kv.first)) return cur;
       if (comp(key, cur->kv.first)) cur = cur->l; else cur = cur->r;
     }
     return nullptr;
   }

   template<typename V>
   pair<Node*, bool> treap_insert(const Key &key, const V &val) {
     if (!root) {
       Node *n = new Node(value_type(key, val), rng_seed(), nullptr);
       root = n; sz = 1; return pair<Node*, bool>(n, true);
     }
     Node *cur = root, *par = nullptr;
     bool go_left = false;
     while (cur) {
       if (eq_key(key, cur->kv.first)) return pair<Node*, bool>(cur, false);
       par = cur;
       if (comp(key, cur->kv.first)) { go_left = true; cur = cur->l; } else { go_left = false; cur = cur->r; }
     }
     Node *n = new Node(value_type(key, val), rng_seed(), par);
     if (go_left) par->l = n; else par->r = n;
     // bubble up by priority (max-heap)
     while (n->p && n->p->pri < n->pri) {
       if (n->p->l == n) rotate_right(n->p); else rotate_left(n->p);
     }
     ++sz; return pair<Node*, bool>(n, true);
   }

   void erase_node(Node *x) {
     if (!x) return;
     // rotate down until leaf
     while (x->l || x->r) {
       if (!x->l) rotate_left(x);
       else if (!x->r) rotate_right(x);
       else {
         if (x->l->pri < x->r->pri) rotate_left(x); else rotate_right(x);
       }
     }
     // now leaf
     if (!x->p) { root = nullptr; }
     else if (x->p->l == x) x->p->l = nullptr; else x->p->r = nullptr;
     delete x; --sz;
   }

   void clear_nodes(Node *x) {
     if (!x) return;
     clear_nodes(x->l); clear_nodes(x->r); delete x;
   }

  public:
   class const_iterator;
   class iterator {
     friend class map;
     friend class const_iterator;
     private:
       Node *ptr;
       map *owner;
       bool at_end;
       iterator(Node *p, map *o, bool e): ptr(p), owner(o), at_end(e) {}
     public:
       iterator(): ptr(nullptr), owner(nullptr), at_end(true) {}
       iterator(const iterator &other) = default;

       iterator operator++(int) {
         if (!owner || at_end) throw invalid_iterator();
         iterator tmp(*this);
         ++(*this);
         return tmp;
       }
       iterator &operator++() {
         if (!owner || at_end) throw invalid_iterator();
         if (ptr->r) { ptr = owner->leftmost(ptr->r); return *this; }
         Node *p = ptr->p;
         while (p && p->r == ptr) { ptr = p; p = p->p; }
         if (!p) { at_end = true; ptr = nullptr; }
         else ptr = p;
         return *this;
       }

       iterator operator--(int) {
         iterator tmp(*this);
         --(*this);
         return tmp;
       }
       iterator &operator--() {
         if (!owner) throw invalid_iterator();
         if (at_end) {
           if (owner->sz == 0) throw invalid_iterator();
           ptr = owner->rightmost(owner->root); at_end = false; return *this;
         }
         if (ptr->l) { ptr = owner->rightmost(ptr->l); return *this; }
         Node *p = ptr->p;
         while (p && p->l == ptr) { ptr = p; p = p->p; }
         if (!p) throw invalid_iterator();
         ptr = p; return *this;
       }

       value_type &operator*() const {
         if (!owner || at_end || !ptr) throw invalid_iterator();
         return const_cast<value_type&>(ptr->kv);
       }
       value_type *operator->() const noexcept {
         if (!owner || at_end || !ptr) return nullptr;
         return &(const_cast<value_type&>(ptr->kv));
       }

       bool operator==(const iterator &rhs) const { return owner == rhs.owner && at_end == rhs.at_end && ptr == rhs.ptr; }
       bool operator!=(const iterator &rhs) const { return !(*this == rhs); }
       bool operator==(const const_iterator &rhs) const { return owner == rhs.owner && at_end == rhs.at_end && ptr == rhs.ptr; }
       bool operator!=(const const_iterator &rhs) const { return !(*this == rhs); }
   };

   class const_iterator {
     friend class map;
     private:
       const Node *ptr;
       const map *owner;
       bool at_end;
       const_iterator(const Node *p, const map *o, bool e): ptr(p), owner(o), at_end(e) {}
     public:
       const_iterator(): ptr(nullptr), owner(nullptr), at_end(true) {}
       const_iterator(const const_iterator &other) = default;
       const_iterator(const iterator &other): ptr(other.ptr), owner(other.owner), at_end(other.at_end) {}

       const_iterator operator++(int) {
         if (!owner || at_end) throw invalid_iterator();
         const_iterator tmp(*this);
         ++(*this);
         return tmp;
       }
       const_iterator &operator++() {
         if (!owner || at_end) throw invalid_iterator();
         if (ptr->r) { ptr = owner->leftmost(ptr->r); return *this; }
         const Node *p = ptr->p;
         const Node *c = ptr;
         while (p && p->r == c) { c = p; p = p->p; }
         if (!p) { at_end = true; ptr = nullptr; }
         else ptr = p;
         return *this;
       }

       const_iterator operator--(int) {
         const_iterator tmp(*this);
         --(*this);
         return tmp;
       }
       const_iterator &operator--() {
         if (!owner) throw invalid_iterator();
         if (at_end) {
           if (owner->sz == 0) throw invalid_iterator();
           ptr = owner->rightmost(owner->root); at_end = false; return *this;
         }
         if (ptr->l) { ptr = owner->rightmost(ptr->l); return *this; }
         const Node *p = ptr->p;
         const Node *c = ptr;
         while (p && p->l == c) { c = p; p = p->p; }
         if (!p) throw invalid_iterator();
         ptr = p; return *this;
       }

       const value_type &operator*() const {
         if (!owner || at_end || !ptr) throw invalid_iterator();
         return ptr->kv;
       }
       const value_type *operator->() const noexcept {
         if (!owner || at_end || !ptr) return nullptr;
         return &ptr->kv;
       }

       bool operator==(const const_iterator &rhs) const { return owner == rhs.owner && at_end == rhs.at_end && ptr == rhs.ptr; }
       bool operator!=(const const_iterator &rhs) const { return !(*this == rhs); }
       bool operator==(const iterator &rhs) const { return owner == rhs.owner && at_end == rhs.at_end && ptr == rhs.ptr; }
       bool operator!=(const iterator &rhs) const { return !(*this == rhs); }
   };

   map() {}
   map(const map &other): comp(other.comp) {
     // inorder insert preserves key order
     for (const_iterator it = other.cbegin(); it != other.cend(); ++it) {
       treap_insert(it->first, it->second);
     }
   }

   map &operator=(const map &other) {
     if (this == &other) return *this;
     clear(); comp = other.comp;
     for (const_iterator it = other.cbegin(); it != other.cend(); ++it) {
       treap_insert(it->first, it->second);
     }
     return *this;
   }

   ~map() { clear(); }

   T &at(const Key &key) {
     Node *n = bst_find(key);
     if (!n) throw index_out_of_bound();
     return n->kv.second;
   }
   const T &at(const Key &key) const {
     Node *n = bst_find(key);
     if (!n) throw index_out_of_bound();
     return n->kv.second;
   }

   T &operator[](const Key &key) {
     auto res = treap_insert(key, T());
     return res.first->kv.second;
   }
   const T &operator[](const Key &key) const {
     Node *n = bst_find(key);
     if (!n) throw index_out_of_bound();
     return n->kv.second;
   }

   iterator begin() {
     if (sz == 0) return iterator(nullptr, this, true);
     return iterator(leftmost(root), this, false);
   }
   const_iterator cbegin() const {
     if (sz == 0) return const_iterator(nullptr, this, true);
     return const_iterator(leftmost(root), this, false);
   }

   iterator end() { return iterator(nullptr, this, true); }
   const_iterator cend() const { return const_iterator(nullptr, this, true); }

   bool empty() const { return sz == 0; }
   size_t size() const { return sz; }

   void clear() {
     clear_nodes(root); root = nullptr; sz = 0;
   }

   pair<iterator, bool> insert(const value_type &value) {
     auto res = treap_insert(value.first, value.second);
     return pair<iterator, bool>(iterator(res.first, this, false), res.second);
   }

   void erase(iterator pos) {
     if (pos.owner != this || pos.at_end || !pos.ptr) throw invalid_iterator();
     erase_node(pos.ptr);
   }

   size_t count(const Key &key) const { return bst_find(key) ? 1 : 0; }

   iterator find(const Key &key) {
     Node *n = bst_find(key);
     if (!n) return end();
     return iterator(n, this, false);
   }
   const_iterator find(const Key &key) const {
     Node *n = bst_find(key);
     if (!n) return cend();
     return const_iterator(n, this, false);
   }
};

}

#endif