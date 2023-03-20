# SO-shell-etap2
*Specyfikacja drugiego etapu projektu realizowanego w ramach ćwiczeń do przedmiotu Systemy Operacyjne.*

## Implementacja shella - etap 2 (wejście z pliku).

Zmiany w stosunku do Etapu 1:

1. Wypisujemy prompt na `STDOUT` tylko jeśli `STDIN` odpowiada specjalnemu urządzeniu znakowemu (`man fstat`).
1. Wczytując linię z `STDIN` musimy się liczyć z tym że zostanie wczytanych wiele linii, oraz z tym że ostatnia z nich może nie być wczytana w całości. Należy uwzględnić przypadek, w którym w jednym odczycie zostanie wczytana tylko część linii.
1. Wciąż wystarczy wykonywać tylko pierwszą z komend z każdej z wczytanych linii.
1. Proszę pamiętać że ostania linia wejścia może się zakończyć końcem pliku a nie końcem linii.

Skrypty oraz pierwszy zestaw testów znajdują się w katalogu `test`. Instrukcje uruchamiania testów są w pliku `test/README.md`.

Każdy test z pierwszego zestawu jest wywoływany w dwóch trybach:
- w pierwszym, na wejście podawany jest od razu cały plik,
- w drugim, wejście jest podawane programowi `splitter` i przekazywane przez pipe do shella. `splitter` przepisuje wejście na wyjście robiąc przerwy w pseudolosowych momentach.
