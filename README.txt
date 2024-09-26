Pour compiler l'ordonnanceur LIFO : 
gcc -Wall -o quicksort quicksort.c sched_lifo.c

Pour l'executer: 
./quicksort

Pour compiler l'ordonnanceur par Work Stealing : 
gcc -Wall -o quicksort quicksort.c sched_ws.c

Pour l'executer: 
./quicksort

Pour exécuter 20 tests sur un nombre différent de cœurs (automatisé, chaque variation du cœur est enregistrée dans un fichier séparé).
Pour 4 cœurs:
./run-20-aggregated-data-lifo.sh
./run-20-aggregated-data-ws.sh
Pour 12 cœurs (mac):
./run-20-aggregated-data-lifo-mac.sh 
./run-20-aggregated-data-ws-mac.sh
