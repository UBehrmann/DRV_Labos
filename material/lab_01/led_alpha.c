#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>
#include <stdint.h>

// Adresse mémoire des périphériques (adapter si nécessaire)
#define HW_BASE_ADDR  0xFF200000  // Base d'adresse des périphériques
#define HEX0_OFFSET   0x20        // Offset HEX0
#define LED_OFFSET    0x00        // Offset LEDs
#define KEY_OFFSET    0x50        // Offset des boutons

#define MAP_SIZE 4096UL

// Registres des périphériques
#define HEX0 *(volatile uint32_t *)(hw_base + HEX0_OFFSET)
#define LEDS *(volatile uint32_t *)(hw_base + LED_OFFSET)
#define KEYS *(volatile uint32_t *)(hw_base + KEY_OFFSET)

// Table de conversion ASCII vers 7-segments (A-Z)
const unsigned char ascii_to_7seg[26] = {
    0x77, // A
    0x7C, // B
    0x39, // C
    0x5E, // D
    0x79, // E
    0x71, // F
    0x3D, // G
    0x76, // H
    0x30, // I
    0x1E, // J
    0x75, // K
    0x38, // L
    0x37, // M
    0x54, // N
    0x5C, // O
    0x73, // P
    0x67, // Q
    0x50, // R
    0x6D, // S
    0x78, // T
    0x3E, // U
    0x1C, // V
    0x2A, // W
    0x49, // X
    0x6E, // Y
    0x5B  // z
};

// Pointeurs de registre
volatile unsigned int *hex0, *leds, *keys;
int mem_fd;
void *hw_base;

// Lettre affichée (initialement 'A')
char current_letter = 'A';

// Fonction pour désactiver les affichages et LEDs avant de quitter
void cleanup(int signum) {
    printf("\nArrêt du programme... Extinction des LEDs et de l'affichage.\n");

    // Éteindre les afficheurs 7-segments et LEDs
    HEX0 = 0x00;
    LEDS = 0x00;

    // Libérer la mémoire et fermer /dev/mem
    munmap(hw_base, MAP_SIZE);
    close(mem_fd);

    exit(0);
}

// Met à jour l'affichage HEX0 et les LEDs
void update_display() {
    unsigned char seg_value = ascii_to_7seg[current_letter - 'A'];
    unsigned char ascii_value = (unsigned char)current_letter;

    HEX0 = seg_value;
    LEDS = ascii_value;

    printf("\rAffiché: %c | HEX0: 0x%02X | LEDs: 0b%08b", current_letter, seg_value, ascii_value);
    fflush(stdout);
}

int main() {
    // Ouvrir /dev/mem
    mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (mem_fd < 0) {
        perror("Erreur lors de l'ouverture de /dev/mem");
        return 1;
    }

    // Mapper la mémoire des périphériques
    hw_base = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, HW_BASE_ADDR);
    if (hw_base == MAP_FAILED) {
        perror("Erreur lors du mappage mémoire");
        close(mem_fd);
        return 1;
    }

    // Capture du signal pour un arrêt propre
    signal(SIGINT, cleanup);

    // Affichage initial
    update_display();

    while (1) {
        usleep(100000); // 100ms pour éviter une charge CPU excessive

        unsigned int key_status = KEYS;

        if (key_status & 0x1) {  // KEY0 appuyé
            current_letter = (current_letter == 'A') ? 'Z' : current_letter - 1;
            update_display();
            usleep(300000); // Debounce
        }

        if (key_status & 0x2) {  // KEY1 appuyé
            current_letter = (current_letter == 'Z') ? 'A' : current_letter + 1;
            update_display();
            usleep(300000); // Debounce
        }
    }

    // Cleanup (inatteignable, mais pour la forme)
    cleanup(0);
    return 0;
}
