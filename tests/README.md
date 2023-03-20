# Testy dla projektu *shell*.


1. Przed rozpoczęciem testów w rozpakowanym katalogu `tests` uruchom
```
	./prepare.sh
```
1. Po zakończeniu testów w `tests` uruchom
```
	./clean.sh
```
1. Do uruchomienia wszystkich testów służy `run_all.sh`. Przykładowe wywołanie
```
	./run_all.sh ~/shell/mshell
```
1. Pojedynczy zestaw można uruchomić za pomocą `run_suite.sh`.
1. Pojedynczy test za pomocą `run_one.sh`.

Nazwa katalogu w którym są wykonywane testy jest zdefiniowana w pliku `setup`.
Domyślnie jest to
```
TEST_DIR=/tmp/$USER.ShellTest
```
