<div align="justify" style="margin-right:25px;margin-left:25px">

# Laboratoire 0 : Consolidation du langage C <!-- omit in toc -->

# Auteur <!-- omit in toc -->

Urs Behrmann

# Table des matières <!-- omit in toc -->

- [Exercice 1](#exercice-1)
- [Exercice 2](#exercice-2)
- [Exercice 3](#exercice-3)
- [Exercice 4](#exercice-4)
- [Exercice 5](#exercice-5)
- [Exercise 6](#exercise-6)

# Exercice 1

Il faut ajouter des directives de préprocesseur pour éviter les inclusions multiples. On utilise les directives `#ifndef`, `#define` et `#endif` pour éviter les inclusions multiples. Voici un exemple de code :

```c
#ifndef NOM_DU_FICHIER_H
#define NOM_DU_FICHIER_H

//...

#endif
```

# Exercice 2

**Résultat de l’exécution :**

```bash
$ gcc sizeof_test.c -o sizeof_test -Wall
sizeof_test.c: In function ‘print_size’:
sizeof_test.c:29:22: warning: ‘sizeof’ on array function parameter ‘str_array’ will return size of ‘char *’ [-Wsizeof-array-argument]
   29 |                sizeof(str_array), sizeof(str_out));
      |                      ^
sizeof_test.c:7:22: note: declared here
    7 | void print_size(char str_array[])
      |                 ~~~~~^~~~~~~~~~~
```

**Pourquoi les tailles des tableaux str_array et str_out sont différentes ? Faites le lien avec le warning lors de la compilation.**

Les tailles des tableaux `str_array` et `str_out` sont différentes parce que `str_array` est un pointeur et non un tableau. Lorsqu’on passe un tableau à une fonction, il est automatiquement converti en pointeur. La taille d’un pointeur est de 8 octets sur une architecture 64 bits, alors que la taille d’un tableau est la taille du tableau multipliée par la taille de chaque élément du tableau. Le warning lors de la compilation nous indique que la taille de `str_array` est celle d’un pointeur et non celle d’un tableau.

**Comment pourrais-je savoir la taille du vecteur my_array ? Et s’il s’agissait d’un tableau statique ?**

Pour connaître la taille du vecteur `my_array`, on peut utiliser la fonction `sizeof(my_array) / sizeof(my_array[0])`. Si `my_array` était un tableau statique, on pourrait utiliser la même méthode pour connaître sa taille.

**Quelles sont les différences sur le résultat et pourquoi ?**

```bash	
$ ./sizeof_test
sizeof(int_var) = 4, sizeof(int) = 4
sizeof(float_var) = 4, sizeof(float) = 4
sizeof(double_var) = 8, sizeof(double) = 8
sizeof(char_var) = 1, sizeof(unsigned char) = 1
sizeof(str_ptr) = 8, sizeof(char *) = 8
sizeof(str_array) = 8
sizeof(str_out) = 59
Of course, I can trick the function sizeof() with the appropriate cast...
For instance, sizeof((char)double_var) = 1

For the dynamic allocation, the correct way to do the cast is the following one:
int *my_array = malloc(sizeof(*my_array) * 10);
Why? Should you change the type of 'my_array' in the declaration but not
in the malloc, then the memory allocated by:
int *my_array = malloc(sizeof(int) * 10);
would have the wrong size.

Call result: 0x557abb53e6b0
malloc() returns a void* but no need to cast it explicitly
(but you do have to cast it for printing the resulting pointer)

sizeof(my_array) = 8, sizeof(*my_array) = 4
```

```bash
$ ./sizeof_test
sizeof(int_var) = 4, sizeof(int) = 4
sizeof(float_var) = 4, sizeof(float) = 4
sizeof(double_var) = 8, sizeof(double) = 8
sizeof(char_var) = 1, sizeof(unsigned char) = 1
sizeof(str_ptr) = 4, sizeof(char *) = 4
sizeof(str_array) = 4
sizeof(str_out) = 59
Of course, I can trick the function sizeof() with the appropriate cast...
For instance, sizeof((char)double_var) = 1

For the dynamic allocation, the correct way to do the cast is the following one:
int *my_array = malloc(sizeof(*my_array) * 10);
Why? Should you change the type of 'my_array' in the declaration but not
in the malloc, then the memory allocated by:
int *my_array = malloc(sizeof(int) * 10);
would have the wrong size.

Call result: 0x579ba5b0
malloc() returns a void* but no need to cast it explicitly
(but you do have to cast it for printing the resulting pointer)

sizeof(my_array) = 4, sizeof(*my_array) = 4
```

Les différences sur le résultat sont dues à la taille des pointeurs qui dépend de l’architecture. Sur la première exécution, la taille des pointeurs est de 8 octets, alors que sur la deuxième exécution, la taille des pointeurs est de 4 octets.

# Exercice 3

Écrivez un petit logiciel qui affecte 0xBEEFCAFE à une variable entière, et ensuite allez relire cette valeur en la regardant un byte à la fois dans une boucle for (bien sûr, oubliez l’utilisation des [] !! — tout doit être fait avec les pointeurs). Vous ne remarquez rien dans l’affichage ? Comment pouvez-vous résoudre le/les problèmes ? Que pouvez-vous en conclure sur l’endianness de votre système ?

```bash
gcc -m32 ex3.c -o ex3 -Wall
```

Résultat:

```bash
$ ./ex3
Value in hex: 0xBEEFCAFE
Bytes in memory:
Byte 0: 0xFE
Byte 1: 0xCA
Byte 2: 0xEF
Byte 3: 0xBE
```

# Exercice 4

**Vous trouverez dans l’archive du labo un exemple de programmation OOP fait en langage C. Ajoutez à cet exemple le parallélépipède rectangle, avec une fonction pour en calculer le volume. On part du principe qu’on ne peut pas changer la position le long de l’axe z (sinon on serait un peu embêtés avec la fonction _moveTo()) (pourquoi ???).**

Parce qu'il n'y a pas d'axe z dans les formes parents, donc la fonction _moveTo() ne peut pas être utilisée pour déplacer un objet le long de l'axe z.

Il faudrait adapter les classes parentes pour ajouter un axe z, et donc permettre de déplacer un objet le long de cet axe.

# Exercice 5

**Output:**

```bash
$ ./ex5
Data1 - Number: 42, Character: A
Data2 - Number: 24, Character: B
```

# Exercise 6

Regardez dans les entêtes du noyau Linux (ce ne serait pas mal de pré-filtrer la recherche avec git-grep en utilisant la bonne option… à vous de trouver laquelle… ;)) quels attributs sont utilisés et leur signification dans le manuel GCC. Résumez ensuite par écrit ce que vous avez observé.


</div>