#include <stdint.h>
#include <stdbool.h>
/**
 * Zobrazi fraktal v novem vlakne
 * */
void *thread_fractal(void *args);

/**
 * Vygeneruje julia set dle danych parametru
 * */
uint16_t *
make_fractal(int width, int height, double move_x, double move_y, double c_real, double c_imag, int max_iterations);


/**
 * Vytvori animaci fraktalu
 * */
void make_animation();


