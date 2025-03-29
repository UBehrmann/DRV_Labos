#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

// Adresse mémoire des périphériques (adapter si nécessaire)
#define HW_BASE_ADDR 0xFF200000  // Base d'adresse des périphériques
#define HEX0_3_OFFSET 0x20       // Offset HEX0
#define HEX4_5_OFFSET 0x30       // Offset HEX1
#define LED_OFFSET 0x00          // Offset LEDs
#define KEY_OFFSET 0x50          // Offset des boutons
#define KEY_MASK_OFFSET 0x58     // Offset du masque des interruptions
#define KEY_EDGE_OFFSET 0x5C     // Offset du registre d'interruptions

// Registres des périphériques
#define HEX0_3 *(volatile uint32_t *)(hw_base + HEX0_3_OFFSET)
#define HEX4_5 *(volatile uint32_t *)(hw_base + HEX4_5_OFFSET)
#define LEDS *(volatile uint32_t *)(hw_base + LED_OFFSET)
#define KEYS *(volatile uint32_t *)(hw_base + KEY_OFFSET)
#define KEY_MASK         *(volatile uint32_t *)(hw_base + KEY_MASK_OFFSET)
#define KEY_EDGE         *(volatile uint32_t *)(hw_base + KEY_EDGE_OFFSET)

#define SEG_TABLE_SIZE 27

#define KEY_0 0x1
#define KEY_1 0x2
#define KEY_2 0x4
#define KEY_3 0x8

// Table de conversion ASCII vers 7-segments (A-Z)
const unsigned char ascii_to_7seg[SEG_TABLE_SIZE] = {
    0x00,  // ' ' (blank)
    0x77,  // A
    0x7C,  // B
    0x39,  // C
    0x5E,  // D
    0x79,  // E
    0x71,  // F
    0x3D,  // G
    0x76,  // H
    0x30,  // I
    0x1E,  // J
    0x75,  // K
    0x38,  // L
    0x37,  // M
    0x54,  // N
    0x5C,  // O
    0x73,  // P
    0x67,  // Q
    0x50,  // R
    0x6D,  // S
    0x78,  // T
    0x3E,  // U
    0x1C,  // V
    0x2A,  // W
    0x49,  // X
    0x6E,  // Y
    0x5B   // z
};

// Macro pour obtenir le caractère à partir de l'index dans le texte
#define CHAR_FROM_TEXT(x) ((text[x] == 0) ? ' ' : ('A' + text[x] - 1))

static int uio_fd;
static void *hw_base;

// Texte édité (initialement vide)
static unsigned text[6] = {0, 0, 0, 0, 0, 0};

// Curseur d'édition (commence à HEX5)
static int cursor_position = 0;

// Fonction pour désactiver les affichages et LEDs avant de quitter
static void cleanup(int signum) {
    printf("\nArrêt du programme... Extinction des LEDs et de l'affichage.\n");

    // Éteindre les afficheurs 7-segments et LEDs
    HEX0_3 = 0x00;
    HEX4_5 = 0x00;
    LEDS = 0x00;

    // Libérer la mémoire et fermer /dev/mem
    munmap(hw_base, getpagesize());
    close(uio_fd);

    exit(0);
}

// Met à jour l'affichage HEX0-HEX5 et les LEDs
static void update_display() {
    // Hex0-3
    for (int i = 0; i < 4; i++) {
        unsigned char seg_value =
            (text[i] == 0) ? ascii_to_7seg[0] : ascii_to_7seg[text[i]];
        HEX0_3 = (HEX0_3 & ~(0xFF << (i * 8))) | (seg_value << (i * 8));
    }

    // Hex4-5
    for (int i = 0; i < 2; i++) {
        unsigned char seg_value =
            (text[i + 4] == 0) ? ascii_to_7seg[0] : ascii_to_7seg[text[i + 4]];
        HEX4_5 = (HEX4_5 & ~(0xFF << (i * 8))) | (seg_value << (i * 8));
    }

    LEDS = 1 << cursor_position;

    printf("\rTexte: %c%c%c%c%c%c | Curseur: HEX%d", CHAR_FROM_TEXT(5),
           CHAR_FROM_TEXT(4), CHAR_FROM_TEXT(3), CHAR_FROM_TEXT(2),
           CHAR_FROM_TEXT(1), CHAR_FROM_TEXT(0), 5 - cursor_position);

    fflush(stdout);
}

int main() {

    // Ouvrir uio0
    uio_fd = open("/dev/uio0", O_RDWR);
    if (uio_fd < 0) {
        perror("Erreur lors de l'ouverture de /dev/uio0");
        exit(EXIT_FAILURE);
    }

    // Mapper la mémoire des périphériques (offset = 0 pour UIO)
    hw_base =
        mmap(NULL, getpagesize(), PROT_READ | PROT_WRITE, MAP_SHARED, uio_fd, 0);
    if (hw_base == MAP_FAILED) {
        perror("Erreur lors du mappage mémoire");
        close(uio_fd);
        return 1;
    }

    // Active les interruptions pour les boutons
    KEY_MASK = 0xF;

    // Capture du signal pour un arrêt propre
    signal(SIGINT, cleanup);

    // Affichage initial
    update_display();

    while (1) {
        uint32_t info = 1;  // Unmask interrupts

        // Activer les interruptions
        ssize_t nb = write(uio_fd, &info, sizeof(info));
        if (nb != (ssize_t)sizeof(info)) {
            perror("Erreur lors de l'écriture dans uio_fd");
            close(uio_fd);
            exit(EXIT_FAILURE);
        }

        // Utiliser select pour attendre une interruption
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(uio_fd, &fds);

        int ret = select(uio_fd + 1, &fds, NULL, NULL, NULL);
        if (ret > 0 && FD_ISSET(uio_fd, &fds)) {
            nb = read(uio_fd, &info, sizeof(info));
            if (nb == (ssize_t)sizeof(info)) {
                unsigned int key_status = KEY_EDGE;

                if (key_status & KEY_0) {  // KEY0 appuyé
                    if (text[cursor_position] != 0) text[cursor_position]--;
                    update_display();
                }

                if (key_status & KEY_1) {  // KEY1 appuyé
                    if (text[cursor_position] != SEG_TABLE_SIZE - 1)
                        text[cursor_position]++;
                    update_display();
                }

                if (key_status & KEY_2) {  // KEY2 appuyé
                    if (cursor_position < 5) cursor_position++;
                    update_display();
                }

                if (key_status & KEY_3) {  // KEY3 appuyé
                    if (cursor_position > 0) cursor_position--;
                    update_display();
                }

                // Clear l'interruptions
                KEY_EDGE = 0xF;
            } else {
                perror("Erreur lors de la lecture dans uio_fd");
                close(uio_fd);
                exit(EXIT_FAILURE);
            }
        } else {
            perror("Erreur lors de l'attente avec select");
            close(uio_fd);
            exit(EXIT_FAILURE);
        }
    }

    // Cleanup (inatteignable, mais pour la forme)
    cleanup(0);
    return 0;
}
