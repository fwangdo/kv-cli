# Mini KV Store

A terminal-based key-value store written in C++17.
Data persists to a file so it survives restarts.

## Usage

```bash
./kvstore set name alice    # store a key-value pair
./kvstore get name          # retrieve a value
./kvstore delete name       # remove a key
./kvstore list              # show all pairs (sorted)
./kvstore count             # number of stored keys
./kvstore export json       # export as JSON
./kvstore export csv        # export as CSV
```

## Build

```bash
cd practice/mini-kvstore
cmake -B build -S .
cmake --build build
```

## Project Structure

```
practice/mini-kvstore/
├── CMakeLists.txt
├── include/
│   ├── store.h        # KVStore class (data + file I/O)
│   ├── command.h      # Command interface + concrete commands
│   └── exporter.h     # Exporter interface + JSON/CSV
├── src/
│   ├── main.cpp       # CLI parsing + command dispatch
│   ├── store.cpp      # KVStore implementation
│   ├── command.cpp    # Command implementations
│   └── exporter.cpp   # Exporter implementations
└── test.sh            # Manual test script
```

## CS & C++ Concepts Practiced

- **Command Pattern** — encapsulate each CLI action as an object
- **Factory Pattern** — `parseCommand()`, `createExporter()`
- **Strategy Pattern** — swap export formats via polymorphism
- **Virtual Interface** — `Command` and `Exporter` base classes
- **RAII** — file I/O with `ifstream`/`ofstream`
- **`std::optional`** — safe "not found" returns
- **`std::unique_ptr`** — ownership without manual `delete`
- **`std::map`** — sorted associative container (red-black tree)
- **Header/implementation separation** — `.h` declarations, `.cpp` definitions
