.. _laboratoire0:

#############################################
Laboratoire 0 --- Consolidation du langage C
#############################################

.. only:: latex

   .. figure:: ../images/banner_drv.png
      :width: 16cm
      :align: center


Ces pages vous donneront un aperçu des concepts de programmation C avancés (ou
pas ??) qui sont couramment utilisés dans le noyau Linux.
Les connaître vous aidera à :

1. mieux comprendre le noyau lorsque vous fouillez dans ses répertoires --- il peut être parfois (très souvent ??) cryptique
2. éviter des pièges assez méchants à déboguer par la suite.

On discutera aussi du style de programmation à adopter et des différents outils
qui pourraient vous faciliter la vie...

.. raw:: latex

    \newpage

***********************************************************************************
Matériel nécessaire
***********************************************************************************

Dans ce laboratoire on fouille dans les sources du noyau Linux.
Vous devez donc le cloner avec les commandes :

.. code-block:: console

    $ git clone --depth=1 -b socfpga-6.1.55-lts https://github.com/altera-opensource/linux-socfpga.git

L'option :bash:`--depth` limite la profondeur, c'est-à-dire qu'uniquement les n derniers commits sont récupérés.
L'historique est tronqué, mais on évite de gaspiller du temps et de l'espace disque.

Ce dépôt n'est pas le dépôt Linux principal, mais comme nous utilisons du matériel Altera, ce dépôt offre un support matériel maximum.

Il y a aussi un répertoire contenant de petits exercices que vous trouvez dans (:file:`material/lab_00/`).

***********************************************************************************
Style de programmation
***********************************************************************************

Le noyau Linux étant le travail de milliers de personnes (et ayant une taille
avoisinant les 20 millions de lignes de code), il a été rapidement clair qu'il
fallait imposer un style de programmation assez strictes.

Vous trouverez le document "officiel"
`ici <https://www.kernel.org/doc/html/latest/process/coding-style.html>`__.

Dans cette section, je vous détaille (en anglais, désolé...) les points (à mon avis) les plus importants.
N'hésitez pas à revenir sur cette liste au cours de votre progression dans le
cours (il y a des points qui sont pour l'instant très obscurs, mais qui
deviendront bientôt assez clairs !).

* 8-character tab indentation, 80 columns lines but not for printk messages
* use snake_case for names, not camelCase
* [when declaring a pointer] use * adjacent to name and not to type (ex. :c:`char *my_variable;`)
* no typedefs
* functions -> short and do ONE thing
* goto names -> what goto does
* comments -> WHAT the code does and WHY, NOT HOW !!
* one variable declaration per line
* [in data structures used outside a single-threaded environment] ALWAYS use reference count

    * locking -> for keeping structures coherent
    * ref counting -> memory management technique

  if another thread can find data structure and no reference count -> almost always bug!
* use inline functions instead of macros when possible
* enclose macros in :c:`do { } while (0)`
* when printing messages associated to a device -> use :c:`dev_X()` instead of :c:`pr_X()`
* when allocating the memory, use :c:`p = kmalloc(sizeof(*p), ...);`
* when allocating an array, use :c:`p = kmalloc_array(n, sizeof(...), ...);`
* allocation functions all emit a stack dump on failure -> there is no use in
  emitting an additional failure message when :c:`NULL` is returned
* if the name of a function is an action or an imperative command -> function should return an error-code integer
* if the name is a predicate -> function should return a "succeeded" boolean
* in structures do not use bool if cache line layout or size of the value matters (its size and
  alignment varies based on the compiled architecture)

De plus, lorsque cela est possible, utilisez les macros définies dans
:file:`include/linux/kernel.h`.
Exemple :

.. code-block:: C

    #define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
    #define swap(a, b) \
        do { typeof(a) __tmp = (a); (a) = (b); (b) = __tmp; } while (0)
    #define clamp(val, lo, hi) min((typeof(val))max(val, lo), hi)

    #define __CONCAT(a, b) a ## b
    #define CONCATENATE(a, b) __CONCAT(a, b)

Une partie de ces conventions peuvent être vérifiées à l'aide de l'outil présenté dans :ref:`helper`

***********************************************************************************
Include guards
***********************************************************************************

Pour éviter d'inclure plusieurs fois un fichier d'entête lors de la compilation,
on utilise des :c:`#include` guards.

Par exemple, dans le fichier :file:`my_head.h` on écrit :

.. code-block:: C

    #ifndef MY_HEAD_H
    #define MY_HEAD_H

    // file content

    #endif // MY_HEAD_H

https://en.wikipedia.org/wiki/Include_guard

GCC permet aussi d'utiliser :c:`#pragma once` (qui, par contre, n'est pas standard) à la place
de tout cela (plus d'info `ici <https://en.wikipedia.org/wiki/Pragma_once>`__),
mais il n'est pas utilisé dans le kernel (il y a en effet quelques problèmes
dans son utilisation --- c'est assez compliqué de déterminer de manière portable
si deux fichiers sont "physiquement" le même fichier ou pas).

.. admonition:: **Exercice 1**

    Essayez de compiler le logiciel dans le répertoire
    :file:`multiple_files` (qui est dans l'archive du labo) avec la commande
    :bash:`gcc main.c -o main -Wall`
    Qu'est-ce qui se passe ? Corrigez les problèmes qui empêchent la
    compilation. Il y a en effet deux problèmes différents
    qui sont "magiquement" résolus en utilisant "la bonne technique"...
    lesquels ?

***********************************************************************************
Types de variables
***********************************************************************************

La norme du langage C définit la taille minimale pour les types de base, mais
un compilateur peut augmenter cette valeur (p. ex., on pourrait
avoir des variables entières sur 12 bytes, cela n'est pas interdit...)

Puisque nous allons travailler à très bas niveau, il est nécessaire de savoir le
nombre de bits qu'on utilise. Pour cela le fichier d'entête :file:`stdint.h`
définit des types comme :c:`uint32_t`, :c:`uint8_t`, ...
Comme le noyau Linux date d'avant cet entête (bien qu'elle soit supportée dans les
noyaux modernes) vous pouvez aussi utiliser des types comme :c:`u32`, :c:`u8`, ...

Lorsque le nombre de bits n'est pas important (p. ex., si vous avez un compteur
qui boucle un nombre bien défini de fois), vous pouvez utiliser les types de base
(p. ex., :c:`unsigned int`).

sizeof()
========

On doit parfois travailler avec des types dont on ne connaît pas forcément la
taille --- ou, encore pire, on pense la connaître, mais pour ce compilateur /
architecture elle est différente...

Pour nous aider dans cette situation, on peut utiliser la fonction :c:`sizeof()`
qui nous fournit, pour un type ou une variable donnée, sa taille en bytes.

.. admonition:: **Exercice 2**

    Compilez le logiciel dans le répertoire
    :file:`sizeof` avec la commande
    :bash:`gcc sizeof_test.c -o sizeof_test -Wall` (un warning devrait apparaître) et exécuter le.

    * Quel est le résultat de l'exécution ?
    * Pourquoi les tailles des tableaux :c:`str_array` et :c:`str_out` sont différentes ? Faites le lien avec le warning lors de la compilation.
    * Comment pourrais-je savoir la taille du vecteur :c:`my_array` ? Et s'il s'agissait d'un tableau statique ?

    Corriger le code pour que la compilation ne fasse plus de warning, sans changer son résultat final.

    Maintenant forcez la compilation à 32-bit avec la commande
    :bash:`gcc -m32 sizeof_test.c -o sizeof_test -Wall`.
    Exécutez à nouveau le binaire.

    (si le compilateur vous donne un *"fatal error"*, essayez d'installer
    *gcc-multilib* avec :bash:`sudo apt install gcc-multilib`).

    Quelles sont les différences sur le résultat et pourquoi ?

***********************************************************************************
Endianness
***********************************************************************************

Lorsqu'on travaille à bas niveau, il est impératif de savoir comment les données
sont représentées en mémoire : le fameux problème de "l'endianness".

.. figure:: images/endianness.jpg
   :width: 20cm
   :align: center

Il n'est pas évident de prévoir quelle représentation est utilisée, car cela
dépend de l'architecture :

* x84 -> little-endian
* 68k -> big-endian
* ARM -> ???? (au début little-endian, puis big-endian p. ex. pour les MCUs réseau)
* ...

Vous ne voulez pas le savoir non plus, car cela limiterait la portabilité de
votre code. Dans le noyau il y a des fonctions dédiées à gérer cela (voir
`ici <https://kernelnewbies.org/EndianIssues>`__).
Dans la vie réelle, on affiche (en hex) une valeur lue depuis la mémoire et on regarde
ce qui est affiché... ;)

***********************************************************************************
Pointeurs
***********************************************************************************

Le langage C permet d'avoir des pointeurs vers des variables et vers des fonctions.
De plus, il utilise des pointeurs pour se référer à des vecteurs (c.-à-d.., le
nom du vecteur n'est rien d'autre qu'un pointeur sur la région de mémoire que ---
on espère, au moins --- est réservée pour les données du vecteur).

Les pointeurs en C sont **typés** : p. ex., un pointeur sur un entier n'est pas la
même chose qu'un pointeur sur un caractère, et cela, même si les deux "variables
pointeurs" occupent la même quantité de mémoire (32 ou 64 bits, selon
l'architecture).
Cela est dû au fait qu'il y a une arithmétique des pointeurs : lorsque j'ai un
pointeur sur une variable entière et que je l'incrémente (pour passer à l'entier suivant
du vecteur), je fais juste "+1" mais en interne cela se traduit par un
"+4 bytes", alors que dans le cas des caractères, c'est juste "+1 byte".
En effet, si vous vous souvenez de :c:`sizeof()`, cela est exactement la quantité
qui est utilisée pour l'incrément.

Le compilateur va essayer de nous aider : si on utilise un pointeur vers un
caractère pour se référer à une variable entière, il va émettre un avertissement.
Le langage C étant bas-niveau, il est possible de faire disparaître ces
avertissements en faisant des cast, p. ex. :

.. code-block:: C

    int myint;
    char *cptr = (char *)&myint;

Il faut, par contre, être sûr de ce qu'on fait...

.. admonition:: **Exercice 3**

    Écrivez un petit logiciel qui affecte 0xBEEFCAFE à une variable entière,
    et ensuite allez relire cette valeur en la regardant un byte à la fois
    dans une boucle for (bien sûr, oubliez l'utilisation des :c:`[]` !! ---
    tout doit être fait avec les pointeurs).
    Vous ne remarquez rien dans l'affichage ?
    Comment pouvez-vous résoudre le/les problèmes ?
    Que pouvez-vous en conclure sur l'endianness de votre système ?

Pointeurs vers fonction
=======================

Dans le langage C, on a la possibilité de déclarer des pointeurs vers des fonctions.
Cela s'avère utile lorsqu'on veut changer au run-time le comportement --- émulant
ce qui est fait dans le langage C++.
Vous trouvez davantage d'informations concernant les pointeurs vers des fonctions
`ici <https://www.cprogramming.com/tutorial/function-pointers.html>`__.

Pointeurs vers void*
====================

Un pointeur de type :c:`void` n'a pas de type associé. Il est donc impossible de
l'incrémenter / décrémenter (même si le GCC a arbitrairement attribué à
:c:`void` une taille de 1), et aussi de les déréférencer...
À quoi sert-il donc ???

Lorsque vous ne savez pas quel type de pointeur va être retourné, par exemple si
vous créez votre propre version de :c:`malloc()`, vous pouvez retourner un
pointeur vers :c:`void` et il sera implicitement casté dans le type souhaité --- et,
à l'inverse, si une fonction a comme paramètre un pointeur vers
:c:`void` (p. ex., :c:`memcpy()`), vous pouvez lui passer votre pointeur sans cast
(mais veuillez noter que, dans l'exemple de :c:`memcpy()`, vous devez
lui dire combien de bytes sont à copier, car il perd la notion d'élément et donc
de taille de chaque élément).

Programmation OOP (avec les pointeurs)
============================================

Le noyau Linux, malgré le fait que le langage C ne soit pas orienté-objet, est écrit en
utilisant des techniques OOP.
Cela est fait grâce aux pointeurs vers fonction :
l'équivalent d'une classe C++ est, en effet, une struct qui contient des pointeurs
vers fonction.
Parmi les paramètres de ces fonctions, il faut qu'un des paramètres soit un
pointeur vers une structure --- afin qu'il soit possible accéder aux champs de la
structure.

.. admonition:: **Exercice 4**

    Vous trouverez dans l'archive du labo un exemple de programmation OOP
    fait en langage C.
    Ajoutez à cet exemple le parallélépipède rectangle, avec une fonction pour en
    calculer le volume. On part du principe qu'on ne peut pas changer la
    position le long de l'axe *z* (sinon on serait un peu embêtés avec la
    fonction :c:`_moveTo()`) (pourquoi ???).

***********************************************************************************
struct
***********************************************************************************

Une "struct" est une structure de données regroupant plusieurs variables de type
hétérogène. L'exemple typique d'utilisation est pour une entrée dans
votre carnet d'adresses :
c'est sûrement plus pratique d'avoir prénom, nom, date de naissance, ... de chaque
personne "groupés" dans une entité, au
lieu d'avoir des vecteurs séparés (un avec tous les noms, un avec tous les
prénoms, ...) dont on doit garder trace (et donc la personne *N* aura son prénom
dans :c:`prenoms[N]`, son nom dans :c:`noms[N]`, ...) --- cette dernière solution
serait l'équivalent informatique d'avoir un carnet avec juste les prénoms,
un autre avec juste les noms, ...

Description formelle d'une struct
`ici <https://en.cppreference.com/w/c/language/struct>`__.

.. note::

    Y a-t-il une différence entre

    .. code-block:: c

        struct a {
            int b;
            char c;
        };

    et

    .. code-block:: c

        struct {
            int b;
            char c;
        } a;

    ???

    Combien de mémoire est utilisée dans les deux cas (en supposant
    qu'un entier prend 4 bytes et un caractère 1 byte) ?

.. admonition:: **Exercice 5**

    Écrivez un petit logiciel pour stocker un entier et un caractère de vos
    choix dans les deux cas ci-dessus, et qui les affiche ensuite à
    l'écran.

Pointeurs vers structure
========================

Vous pouvez bien sûr avoir un pointeur vers une structure.
Dans ce cas, vous pouvez vous référer aux champs de la structure de deux façons
équivalentes (bien que la deuxième --- avec :c:`->` --- soit à préférer) :

.. code-block:: C

    struct S {
        int b;
        char c;
    };

    struct S my_struct;
    struct S *my_struct_ptr = &my_struct;

    my_struct.b = 123;
    my_struct.c = 'c';

    (*my_struct_ptr).b = 123;
    (*my_struct_ptr).c = 'c';

    my_struct_ptr->b = 123;
    my_struct_ptr->c = 'c';

Dans le cas :c:`(*x).b`, l'utilisation des parenthèses est impératif, car l'opérateur
:c:`.` a la
précédence sur :c:`*` (et donc on essayerait de déréférencer une valeur
entière).

Initialisation des structures
=============================

Vous pouvez initialiser des structures lors de la déclaration.
Il y a plusieurs façons de le faire, mais celle recommandée utilise les
*designated initializers*, c.-à-d. :

.. code-block:: C

    struct S my_struct = {
        .b = 3,
        .c = 'r',
    };

De la même manière, vous pouvez initialiser des structures dans un vecteur, ce qui
est très souvent fait dans le noyau Linux, par exemple :

.. code-block:: C

    /* From drivers/media/i2c/adv7180.c */
    static const struct of_device_id adv7180_of_id[] = {
        { .compatible = "adi,adv7180", },
        { .compatible = "adi,adv7180cp", },
        { .compatible = "adi,adv7180st", },
        { .compatible = "adi,adv7182", },
        { .compatible = "adi,adv7280", },
        { .compatible = "adi,adv7280-m", },
        { .compatible = "adi,adv7281", },
        { .compatible = "adi,adv7281-m", },
        { .compatible = "adi,adv7281-ma", },
        { .compatible = "adi,adv7282", },
        { .compatible = "adi,adv7282-m", },
        { },
    };

Si vous avez des structures imbriquées, le mécanisme reste le même :

.. code-block:: C

    /* From drivers/media/i2c/adv7180.c */
    static struct i2c_driver adv7180_driver = {
        .driver = {
            .name = KBUILD_MODNAME,
            .pm = ADV7180_PM_OPS,
            .of_match_table = of_match_ptr(adv7180_of_id),
        },
        .probe = adv7180_probe,
        .remove = adv7180_remove,
        .id_table = adv7180_id,
    };

(dans l'exemple ci-dessus, :c:`adv7180_probe` et :c:`adv7180_remove` sont des
pointeurs vers fonctions !)

Forward declarations
--------------------

Le compilateur lit chaque fichier du début à la fin, et s'il rencontre quelque
chose (fonction, variable, constante, ...) qu'il ne connaît pas, il s'énerve et va
se plaindre.
Malheureusement on a parfois des dépendances "circulaires", p.ex entre oeuf et
poule : on aimerait dire qu'une poule est sortie d'un oeuf, donc il faudrait déclarer
d'abord l'oeuf et ensuite la poule. Par contre, on aimerait aussi dire que l'oeuf
va devenir une poule...

Pour résoudre ce problème, on utilise ce qu'on appelle une *forward declaration*:

.. code-block:: C

    struct oeuf;

    struct poule {
        struct oeuf *nee_de;
        // [ ... ]
    }
    struct oeuf {
        struct poule *pondu_par;
        // [ ... ]
    }

Compiler attributes
===================

Il est parfois utile de "guider" le compilateur dans ses choix.
Par exemple, par défaut le compilateur essaye de créer des structures bien
alignées, mais parfois on souhaite qu'elles soient bien "denses", sans padding.
Pour cela, on peut utiliser (avec GCC) l'attribut :c:`packed`:

.. code-block:: C

    __attribute__((__packed__))

Pour davantage d'info, regardez le manuel de GCC (
`ici <https://gcc.gnu.org/onlinedocs/gcc-4.0.2/gcc/Type-Attributes.html>`__ et
`ici <https://gcc.gnu.org/onlinedocs/gcc-4.7.2/gcc/Function-Attributes.html>`__).

.. admonition:: **Exercice 6**

    Regardez dans les entêtes du noyau Linux (ce ne serait pas mal de
    pré-filtrer la recherche avec :bash:`git-grep` en utilisant la bonne
    option... à vous de trouver laquelle... ;)) quels attributs sont
    utilisés et leur signification dans le manuel GCC. Résumez ensuite par écrit
    ce que vous avez observé.

offsetof / container_of
=======================

:c:`offsetof` est une macro qui permet d'avoir l'offset (en bytes) d'un champ
dans une struct.

Description formelle `ici <https://en.cppreference.com/w/c/types/offsetof>`__.

Le noyau Linux l'utilise dans une macro, :c:`container_of`, qui est très utilisée
par récupérer le pointeur vers une structure en n'ayant que d'un pointeur sur un champ
de la structure. Vous avez un exemple d'utilisation
`ici <http://www.kroah.com/log/linux/container_of.html>`__.

.. admonition:: **Exercice 7**

    Définissez la macro :c:`container_of` (ci-dessous) et une structure
    contenant trois variables de type différent.

    .. code-block:: C

        #define container_of(ptr, type, member) \
            (type*)(void*)((char*)ptr - offsetof(type, member))

    Ensuite vérifiez que vous arrivez bien à obtenir l'adresse de base de
    la structure à partir de l'adresse de chacun de ses champs (avec la
    macro ci-dessus vous allez probablement avoir une erreur... à vous de
    la résoudre !).

    Que pouvez-vous dire au sujet de l'alignement des champs de la
    structure ?
    Répétez le test lorsque la structure est caractérisée par l'attribute
    *packed*. Pourquoi ces avertissements ? Est-ce que maintenant les
    adresses ont "plus de sens" ?

***********************************************************************************
union
***********************************************************************************

Une union est similaire à une struct dans sa déclaration, mais son comportement
est très différent : au lieu de regrouper tous ses champs, il n'alloue que la
mémoire nécessaire pour le plus grand d'entre eux (evt. avec du padding).
Les champs sont donc utilisés de façon mutuellement exclusive.

Description formelle d'une union
`ici <https://en.cppreference.com/w/c/language/union>`__.

Cela permet de :

* épargner de la mémoire
* gérer des types différents (p. ex., une fonction qui doit lire des bytes et
  des entiers) en C (en C++ on utiliserait p. ex., des templates)

.. note:: Observez la déclaration de :c:`union i2c_smbus_data`.

    Cherchez-la dans les sources du kernel à l'aide p. ex. de :bash:`git-grep`;
    ensuite, regardez comment elle est utilisée par les fonctions de
    lecture/écriture
    décrites `ici <https://www.kernel.org/doc/html/latest/i2c/writing-clients.html#smbus-communication>`__.
    Quelle est l'utilité des :c:`union` dans ce cas ?

Les unions sont aussi utiles pour avoir *"des jolis noms"*, p. ex. (dans la structure :c:`struct snd_soc_dapm_path` contenue dans le fichier
:file:`include/sound/soc-dapm.h`) :

.. code-block:: C

    /*
     * source (input) and sink (output) widgets
     * The union is for convience, since it is a lot nicer to type
     * p->source, rather than p->node[SND_SOC_DAPM_DIR_IN]
     */
    union {
        struct {
            struct snd_soc_dapm_widget *source;
            struct snd_soc_dapm_widget *sink;
        };
        struct snd_soc_dapm_widget *node[2];
    };

Ce dernier est un exemple d'une technique qui s'appelle
`type punning <https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html#Type%2Dpunning>`__.
Elle est possible à partir de la version C99 du langage (avant, c'était undefined
behavior --- voir plus bas), et elle est souvent utilisée dans le noyau.

.. admonition:: **Exercice 8**

    Tout comme l'exercice 3, écrivez un programme qui affecte la valeur 0xBEEFCAFE à une variable et
    ensuite, relisez la valeur byte par byte. Utiliser une union pour cela.

***********************************************************************************
Bit fields
***********************************************************************************

Il est parfois utile de limiter le nombre de bits d'une variable, par exemple à
1. Cela est possible grâce aux bit fields.
Vous trouverez la description formelle `ici <https://en.cppreference.com/w/c/language/bit_field>`__
et des exemples d'utilisation
`ici <https://www.cs.emory.edu/~cheung/Courses/255/Syllabus/2-C-adv-data/bit-field.html>`__.

Très utile pour des gens qui doivent travailler à très bas niveau, n'est-ce pas ?
Mais...

**Caveat emptor!** Lorsque vous utilisez des bit fields, vous devez avoir
confiance dans le fait que le compilateur fasse les bons choix !

En effet, dans la norme, il est écrit :
*"The order of allocation of bit-fields within a unit (high-order to low-order or
low-order to high-order) is implementation-defined."*
(`source <https://c0x.shape-of-code.com/6.7.2.1.html>`__).
De plus, des choses bizarres peuvent se passer avec les caches......
et `cela a posé quelques petits soucis dans le passé :S <https://lwn.net/Articles/478657/>`__

Quelle est donc l'alternative à utiliser ? Le **shift & mask !**

Dans le noyau Linux on peut même utiliser la macro :

.. code-block:: C

    #define BIT(nr)         (1UL << (nr))

si l'on est bien paresseux ;)

Exemple :

.. code-block:: C

    uint32_t x = 0;
    uint32_t const n = 2;
    uint32_t b;
    // test bit n value
    b = (x >> n) & 0x1;
    // set bit n
    x |= BIT(n);
    // clear bit n
    x &= ~BIT(n);
    // toggle bit n
    x ^= BIT(n);

Vous trouverez des jolis "Bit Twiddling Hacks"
`ici <https://graphics.stanford.edu/~seander/bithacks.html>`__.

***********************************************************************************
volatile
***********************************************************************************

Le compilateur, selon les options de compilation, pourrait faire des choses
bizarres avec votre code...

Exemple : vous avez une variable qui correspond à un registre dans votre
périphérique.
Vous affectez à cette variable une valeur, ensuite vous faites autre chose et
vous contrôlez la valeur de cette variable.
Si vous demandez au compilateur d'optimiser votre code, il va se dire : *"mmm,
personne a touché cette variable, donc elle va avoir la même valeur que tout à
l'heure, et donc ..."*. Mais, dans la réalité, votre périphérique pourrait bien
avoir changé la valeur de la variable, et ce changement ne sera pas pris en
compte à cause du compilateur !

Pour éviter cela, le keyword :c:`volatile` a été introduit en C. Il signifie *"ne fais
pas d'hypothèse stp, il faut qu'à chaque fois, tu relises la valeur de cette variable"*.

.. admonition:: **Exercice 9**

    Écrivez une fonction dans `GodBolt <https://godbolt.org/>`__ pour voir par vous-mêmes
    ce que cela veut dire en termes de code assembleur --- c.-à-d.,
    écrivez une fonction qui fait le test susmentionné avec et sans le keyword
    :c:`volatile`, et avec différents niveaux d'optimisation (essayez p. ex., avec
    :bash:`-O0` et :bash:`-O3`).
    Commentez par écrit les résultats.

***********************************************************************************
Undefined behavior
***********************************************************************************

Afin de donner aux développeurs en charge de la création des compilateurs
un peu de marge de manoeuvre, le standard C ne spécifie pas quoi faire dans certaines
situations "limite", p. ex., lorsque vous déréférencez un pointeur à NULL.
Même si cela est sensé, c'est **mal** --- même pire que se coller les doigts
ensemble avec de la colle rapide !
N'avoir pas spécifié quoi faire veut dire que **tout** est possible --- en ordre
décroissant de probabilité :

* un footballeur néo-zélandais très méchant pourrait se matérialiser derrière vous
  et commencer à vous frapper
* le soleil pourrait devenir soudainement une naine blanche
* votre assistant pourrait prononcer avec le bon accent trois mots en italien de suite
* ...

Vous trouverez plus de détails aux liens suivants :

* `A Guide to Undefined Behavior in C and C++ <https://blog.regehr.org/archives/213>`__
* `Undefined behavior can result in time travel (among other things, but time travel is the funkiest) <https://devblogs.microsoft.com/oldnewthing/20140627-00/?p=633>`__
* `C99 list of undefined behaviors <https://gist.github.com/Earnestly/7c903f481ff9d29a3dd1>`__

Heureusement, face à ce danger, des outils ont été crées pour détecter ces
situations.
En particulier, il est possible d'utiliser le **Undefined Behavior Sanitizer (UBSAN)**.
Il suffit, lors de la compilation, d'ajouter le flag :bash:`-fsanitize=undefined`.
Si vous voulez aussi un stack trace il faut d'abord faire
:bash:`export UBSAN_OPTIONS="print_stacktrace=1"`.
Faites gaffe que parfois les erreurs ne sont pas détectées si vous compilez avec
:bash:`-O0` (il faut au moins -O1).
Vous avez plus de détails sur les options possibles
`ici <https://blogs.oracle.com/linux/post/improving-application-security-with-undefinedbehaviorsanitizer-ubsan-and-gcc>`__.
Bien sûr, ces tests sont au run-time et non pas à compile-time (pourquoi ???).

Le Undefined Behavior Sanitizer est utilisé aussi
`dans le noyau <https://www.kernel.org/doc/html/latest/dev-tools/ubsan.html>`__.

.. admonition:: **Exercice 10**

    Écrivez un petit logiciel qui déclare une variable entière sur 32 bits,
    ensuite affectez-lui une valeur.
    Effectuez un shift vers la gauche d'un nombre fixe de bits > 32, par
    exemple 42, et exécutez le logiciel.
    Qu'est-ce qui se passe ? Est-ce que le compilateur vous donnes un avertissement ou
    pas ?
    Et si le nombre de bits à shifter était donné par une variable rentrée par
    l'utilisateur ?

    Regardez dans `GodBolt <https://godbolt.org/>`__ comment le code assembleur
    change avec le code et selon le niveau d'optimisation.

    Essayez ensuite de compiler avez UBSAN activé, observez le comportement du
    système et commentez-le.

*****************************************
Travail à rendre et critères d'évaluation
*****************************************

Dans le cadre de ce laboratoire, vous devez rendre les 10 exercices ci-dessus.

.. include:: ../consigne_rendu.rst.inc
