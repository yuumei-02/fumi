.ONESHELL:
.PHONY: setup fumi

fumi: setup
	cd fumi
	make fumi

setup:
	mkdir -p build/bin
