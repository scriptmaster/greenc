all: builds


builds: tree test
tree:
	nim c -d:release --opt:size tree.nim

test:
	# greenc.exe build ../src/ -o ../build/
