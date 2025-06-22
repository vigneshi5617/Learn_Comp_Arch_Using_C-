# Lab 1 – CacheLine Design

## 🎯 Objective

Design a `CacheLine` struct to hold information about a single block in the cache. This will later be used in sets and full cache implementations.

The struct should:
- Store the **tag** of the address
- Indicate **valid** and **dirty** status bits
- Optionally store the **last used time** for LRU
- Possibly hold **data** (not needed yet, maybe for later)

---

## 🧠 What You’ll Learn

- How to use `struct` in C++
- Bit-level operations for extracting tag/index
- Boolean flags for cache logic
- How this will integrate with future `CacheSet`

---

## 📚 Cadence Concepts to Review

From Cadence slides (PDF you provided):

- ✅ **Structs**: How to define and use (`slide ~20+`)
- ✅ **Bitwise Operations**: Shifts, masks, AND/OR (`slide ~25+`)
- ✅ **Pointers and NULLs**: (Used later for "next level" caches)
- ✅ **Functions**: Pass by value vs. reference

---

## 🧪 Tasks

- [ ] Create a struct named `CacheLine`
- [ ] Add fields: `tag`, `valid`, `dirty`, `lru_counter`
- [ ] Write a function `printLine()` to print all values
- [ ] Add a constructor to initialize everything

---

## 🔁 Build & Test

```bash
g++ lab1_cacheline.cpp -o lab1
./lab1
