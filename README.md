# SBD

## External Sorter

An external sorting algorithm using a K-way merging approach to sort large data files.

Records are sets containing up to 15 integers. They are ordered by the maximum of contained integers.

## ISF

An Indexed Sequential File is a data storage system offering
quick access times and reducing disk operations.

Requires the following files: `index.bin`, `data.bin`, `overflow.bin`.

Commands implemented:

- `list;`
- `show <KEY>;`
- `insert <KEY> { <VALUES> };`
- `update <KEY> { <VALUES> };`
- `sort;`
- `status;`

Values must be comma separated integers (trailing comma is a syntax error).
Multiple commands can be entered at once.
