**Ordonnanceurs LIFO et Work Stealing**

**Description du Projet**

Ce projet implémente deux types d'ordonnanceurs : LIFO (Last In, First Out) et Work Stealing. Le but est de comparer leurs performances en triant des données à l'aide d'un algorithme de tri rapide (quicksort) et en exécutant des tests sur différents nombres de cœurs. 

**Commandes**

**1. Compilation de l'ordonnanceur LIFO**

Utilisez la commande suivante pour compiler l'ordonnanceur LIFO :

```c
gcc -Wall -o quicksort quicksort.c sched_lifo.c
```

**2. Exécution de l'ordonnanceur LIFO**

Pour exécuter le programme avec l'ordonnanceur LIFO, utilisez la commande suivante :

```c
./quicksort
```

**3. Compilation de l'ordonnanceur Work Stealing**

Utilisez la commande suivante pour compiler l'ordonnanceur Work Stealing :

```c
gcc -Wall -o quicksort quicksort.c sched_ws.c
```

**4. Exécution de l'ordonnanceur Work Stealing**

Pour exécuter le programme avec l'ordonnanceur Work Stealing, utilisez la commande suivante :

```c
./quicksort
```

**5. Exécution automatisée de 20 tests avec différents nombres de cœurs**

Ce projet inclut des scripts pour exécuter 20 tests automatisés sur différentes configurations de cœurs et enregistrer les résultats.

**a) Pour 4 cœurs :**

Utilisez la commande suivante pour exécuter les tests avec l'ordonnanceur LIFO :

```sh
./run-20-aggregated-data-lifo.sh
```

Utilisez la commande suivante pour exécuter les tests avec l'ordonnanceur Work Stealing :

```sh
./run-20-aggregated-data-ws.sh
```

**b) Pour 12 cœurs (Mac) :**

Utilisez la commande suivante pour exécuter les tests avec l'ordonnanceur LIFO sur Mac :

```sh
./run-20-aggregated-data-lifo-mac.sh
```

Utilisez la commande suivante pour exécuter les tests avec l'ordonnanceur Work Stealing sur Mac :

```sh
./run-20-aggregated-data-ws-mac.sh
```

**Remarques**

- Assurez-vous de compiler l'ordonnanceur approprié avant d'exécuter les tests.
- Les scripts d'exécution enregistrent automatiquement les résultats des tests dans des fichiers séparés pour chaque variation de cœur.

**Rendu final**

![image](https://portfolio-elumalai.fr/images/os1.png)

![image](https://portfolio-elumalai.fr/images/os2.png)

![image](https://portfolio-elumalai.fr/images/os3.png)
