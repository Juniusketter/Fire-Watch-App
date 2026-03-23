
# FireWatch PR Bundle — Back‑End Search (C++ + SQLite FTS5)

This pull request bundle adds **fast full‑text search** for extinguishers using **SQLite FTS5**, with scoring, snippets, filters, pagination, and multi‑tenant scoping.

## What’s included
- `db/migrations/20260315_fts_extinguishers.sql` — creates `EXTINGUISHER_FTS` + triggers.
- `src/search/SearchService.h/.cpp` — production search service (BM25 + snippet).
- `tests/search/search_extinguisher_tests.cpp` — minimal test exercising search.
- `CMakeLists.addon.txt` — lines to append to your root CMake file.

## Apply this PR
1) **Create a branch**
```bash
git checkout -b feat/search-bar-backend
```

2) **Copy files** into your repo at the matching paths.

3) **Append CMake** lines from `CMakeLists.addon.txt` to your root `CMakeLists.txt`.

4) **Run migration** on your dev database (adds FTS table + triggers):
```bash
sqlite3 FireWatch.db < db/migrations/20260315_fts_extinguishers.sql
```

5) **Build & run tests**
```bash
cmake -S . -B build && cmake --build build --parallel
./build/search_ext_tests
```

6) **Commit & push**
```bash
git add db/migrations src/search tests/search CMakeLists.txt
git commit -m "feat(search): backend FTS5 search for extinguishers + tests"
git push -u origin feat/search-bar-backend
```

## Usage example
```cpp
SearchService svc(db);
SearchFilters f; f.type = std::string("ABC");
Page p{1, 20}; int total=0;
auto hits = svc.searchExtinguishers("TENANT-1", ""north exit"", f, p, &total);
```

## Notes
- If FTS5 is not available in your SQLite build, you can implement a LIKE-based fallback. Contact us to add it in this PR.
- Extend this pattern to **Assignments** and **Reports**, or switch to a **unified FTS** if you prefer a single search bar.
