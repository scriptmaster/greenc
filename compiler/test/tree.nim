import os
import algorithm  # <-- REQUIRED for sort

proc printTree(path: string, prefix: string = "") =
  var entries: seq[string] = @[]

  for kind, file in walkDir(path):
    entries.add(file)

  entries.sort(system.cmp)

  for i, fullPath in entries:
    let isLast = (i == entries.len - 1)
    let name = splitPath(fullPath).tail

    let branch = if isLast: "└── " else: "├── "
    echo prefix & branch & name

    if dirExists(fullPath):
      let newPrefix = prefix & (if isLast: "    " else: "│   ")
      printTree(fullPath, newPrefix)

# ----------------------------

let cwd = getCurrentDir()
echo cwd
printTree(cwd)
