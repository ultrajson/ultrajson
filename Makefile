all:
	python setup.py build_ext -if
	@echo
	@echo
	@echo UNIVERSAL
	HPY_UNIVERSAL=1 python setup.py build_ext -if

test:
	python -m pytest tests/tests.py
	HPY_UNIVERSAL=1 python -m pytest tests/tests.py

.PHONY: benchmark
benchmark:
	python -m benchmark.main
