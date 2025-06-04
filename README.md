# Laboratoire Driver

Fork du projet [drv25_student](https://reds-gitlab.heig-vd.ch/reds-public/drv25_student) pour le cours de *Développement de Driver* à la HEIG-VD.

## Membres du groupe

- Urs Behrmann

# Setup de la DE1-SoC

## Configurer le MODE SELECT (MSEL)

![MODE SELECT (MSEL)](/docs/_images/msel.png)

# Commandes pour les labos

## Fetch des nouveaux Labos

```bash
git fetch upstream
git merge upstream/main -Xtheirs
git push origin main
```

## Connexion à la carte

Connexion UART :

```bash
sudo picocom /dev/ttyUSB0 --b 115200
```

MDP : 'heig' ou rien

Connexion SSH :

```bash
ssh root@192.168.0.2
```

## Compilation et run des exercices

Compilation :

```bash
arm-linux-gnueabihf-gcc-6.4.1 ex1.c -o lab1ex1
```

Mettre dans le répertoire '/export/drv/' et exécuter sur la carte.

```bash
~/drv/lab1ex1
```

# Feedback lab1

Bonsoir,
 
Voici un rapide feedback sur votre code de l'exercice 3 du laboratoire 1:
- Compile OK
- Consigne: fichier nommé ex<n>.c
- Bon code, bien commenté
- Déclarer les variables / fonctions "static" (sauf main())
- Les variables globales pourraient plutôt être passées en paramètre
- Il serait plus robuste / réactif de détecter le "flanc montant"lors de l'appui sur les boutons (transition 0 -> 1) - mais le délai300ms permet de répéter en laissant la touche appuyé alors why not
- MAP_SIZE hardcodé à 4kB, utiliser getpagesize() ou équivalent
- Attention au coding style (utiliser des tab pour l'indentation):https://www.kernel.org/doc/html/v6.1/process/coding-style.html

N'hésitez pas à me demander si vous avez des questions.
Bonne soirée

# Feedback lab2

Etudiants :
Behrmann Urs


Exercice 1 et 3                  :   1.5 / 1.5
......Fonctionnement correcte    :     1 / 1.       Poids : 1.0
......UIO utilisé correctement   :     1 / 1.       Poids : 1.0

Exercice 2                       :  0.15 / 0.5
......Question 1                 :     1 / 1.       Poids : 3.0
......Question 2                 :     0 / 1.       Poids : 1.0
...... Question 3                :     0 / 1.       Poids : 6.0

Exercice 4-5                     :  1.33 / 2.0
......Description 3 façons (ex5) :     0 / 1.       Poids : 1.0
......Gestion des boutons        :     1 / 1.       Poids : 1.0
......Interruptions              :     1 / 1.       Poids : 1.0

Code                             :  0.83 / 1.0
......Code compile               :     1 / 1.       Poids : 1.0
......Code propre                :  0.75 / 1.       Poids : 2.0
......Code robuste               :  0.75 / 1.       Poids : 2.0
......Code commenté              :     1 / 1.       Poids : 1.0

Bonus / Malus                    :   0.0 / 5.0


Note finale                      : 4.8


Remarque assistant: 
Exercice 1:
  - Attention aux nombres magiques pour les maskage des boutons
  - Le debounce aurait pu être fait d'une différente manière, car si l'ont appuye sur plusieurs boutons en même temps, le temps d'attente va se cumuler

Exercice 2:
  - README.md pas dans le rendu, pris sur GitHub
  - Manque réponse question 2 et 3

Exercice 4-5:
  - En cas d'erreur lors du `read/poll/select` toutes les ressources ne sont pas libérée correctement
  - Il faudrait aussi désactiver les interruptions du périphérique à la fin du programme (écrire 0 dans KEY_MASK)
  - Il faudrait éviter de mettre l'instruction qui suit un if sur la même ligne. Ça prête à confusion.
  - Attention à ne faire pas trop de niveau d'indentation. Le pattern *early return* pourrait aider dans ce cas là. Exemple simple pour select:

    ```
    while(...) {
        int ret = select(...);
   
        if (ret <= 0) {
            // Error handling (exit or continue)
        }

        if (!FD_ISSET(uio_fd, &fds)) {
            // Error handling (exit or continue)
        }

        if (read(...) != sizeof(info)) {
            // Error handling (exit or continue)
        }

        // Everything OK, can handle key press
    }
    ```



# Feedback lab5

Etudiants :
Behrmann Urs


Module                         :   1.0 / 1.0
......Fonctionnement correcte  :     1 / 1.       Poids : 2.0
......Sysfs                    :     1 / 1.       Poids : 2.0
......Insérer/retirer          :     1 / 1.       Poids : 1.0

Implémentation                 :   1.5 / 1.5
......kfifo                    :     1 / 1.       Poids : 1.0
......kthread                  :     1 / 1.       Poids : 1.0
......ktimer                   :     1 / 1.       Poids : 1.0
......Sysfs                    :     1 / 1.       Poids : 1.0

Synchronisation                :  1.12 / 1.5
......Device ↔kthread ↔ ktimer :   0.5 / 1.       Poids : 1.0
......Protection des variables :  0.75 / 1.       Poids : 2.0
......Variable atomique        :     1 / 1.       Poids : 1.0
......Spinlock                 :  0.75 / 1.       Poids : 1.0
......Mutex                    :  0.75 / 1.       Poids : 1.0

Code                           :  0.71 / 1.0
......Code compile             :  0.75 / 1.       Poids : 1.0
......Code propre              :   0.5 / 1.       Poids : 2.0
......Code robuste             :  0.75 / 1.       Poids : 2.0
......Code commenté            :     1 / 1.       Poids : 1.0

Bonus / Malus                  :   0.0 / 5.0


Note finale                    : 5.3


Remarque assistant: 
Seule la version finale était attendue, correction uniquement du fichier `lab5ex4.c`

Implémentation:
  - FYI, il est possible d'utiliser `sysfs_emit_at` pour écrire à une position données dans le buffer (pour `sequence_show`)

Synchronisation :
  - La synchronisation entre le device char et le thread aurait du se faire avec une variable de complétion pour ne pas faire une attente active sur l'entrée utilisateur.
  - Il faut appeler `spin_lock_init` pour initialiser les spin lock
  - `kfifo_count_show` ne protège pas l'accès à `sequence_fifo`
  - `interval_ms` pourrait également être protégé. (variable atomique)
  - `ktimer` exécute les callbacks en tant que softirq, c'est à dire dans un context d'interruption. Il est donc interdit de bloquer et donc d'utiliser des mutex dedans.

Code:
  - Warning à la compilation
  - Ne pas faire de variable global. Faire une structure privée et utiliser le pointeur générique dans `struct device` et `struct file` pour y accéder. Voir les différents tutos.
  - En cas d'erreur lors du `probe`, les resources déjà allouées (misc device, sysfs files) ne sont pas forcément libérées correctement.
  - Lorsque le module est retiré, il se pourrait que entre l'appel de `complete` et `kthread_stop`, le thread avance dans sont executions et passe la vérification de `kthread_should_stop` qui n'est donc pas encore marqué comme tel. Le thread peut alors se rendormir sur `wait_for_completion` et ne jamais se terminer. Dans ce cas, il faudrait plutôt utiliser `wait_for_completion_interruptible`, qui permet de se réveiller sans avoir à faire complete lorsque `kthread_stop` est appelé. Ou alors, ajouter une variable pour remplacer le système mis en place par `kthread_stop/kthread_should_stop`
