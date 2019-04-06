#include "main.h"

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>

#include "mzapo_phys.h"
#include "mzapo_regs.h"
#include "mzapo_parlcd.h"

#define WIDTH 480
#define HEIGHT 320
#define START_DEPTH 150

//bod pro zobrazovani na lcd displej
unsigned char *parlcd_mem_base;

//bod pro brani hodnot z tlacitek
unsigned char *mem_base;

int main(int argc, char *argv[]) {
    uint32_t rgb_buts;
    //imginarni a realne casti pro zajimave fraktaly
	double re_choose[] = {-0.4, -0.70176, -0.835, -0.8,  0.285, 0.285, 0.45};
	double im_choose[] = {0.6, -0.3842, 0.2321, 0.156,  0, 0.01, 0.1428};
	
    //nacteni displeje a tlacitek
    mem_base = map_phys_address(SPILED_REG_BASE_PHYS, SPILED_REG_SIZE, 0); // tlacitka
    parlcd_mem_base = map_phys_address(PARLCD_REG_BASE_PHYS, PARLCD_REG_SIZE, 0); //displej
    if (mem_base == NULL || parlcd_mem_base == NULL) {
        printf("Error in memory\n");
        exit(1);
    }
    parlcd_hx8357_init(parlcd_mem_base);

    //pocatecni nastaveni pozice a hloubky fraktalu
    uint32_t depth = START_DEPTH;
    double coord_x = 0, coord_y = 0;

    //parametry pro fraktal
    uint32_t red_but = 0, green_but = 0, blue_but = 0;
	bool red_click = false, green_click = false;
	int index = 0;
    uint16_t *fractal;
	double real = re_choose[index];
    double imaginary = im_choose[index];

	
	//vlakno na vykresleni
    pthread_t drawing;
    
    while (true) {
        rgb_buts = *(volatile uint32_t *) (mem_base + SPILED_REG_KNOBS_8BIT_o); //hodnoty rgb

        red_but = (rgb_buts & 0x00FF0000) >> 16; // hodnota cerveneho tlacitka
        green_but = (rgb_buts & 0x0000FF00) >> 8; // hodnota zeleneho tlacitka
		blue_but = (rgb_buts & 0x000000FF); // hodnota modreho tlacitka
		
		red_click = (rgb_buts & 0xFF000000) >> 24 == 4;//zda je kliknute
		green_click = (rgb_buts & 0xFF000000) >> 24 == 2;

	if(!red_click && !green_click){
		//coord_x posouvam pomoci cerveneho
		//coord_y pomoci zeleneho
		coord_x = 0.02 * red_but - 3;
		coord_y = 0.015 * green_but - 2;
		//hloubka pomoci modreho
		depth =2.3 * blue;
		printf("x = %f, y = %f, depth = %u\n", coord_x, coord_y, depth);
	}else if(red_click){
		//po stisknuti zmeni imaginarni a realnou cast
		index = (index +1)%6;
		real = re_choose[index];
		imaginary = im_choose[index];
	}else if(green_click){
		printf("KALSKDLASKDLASKLDKSA\n");
		make_animation();
	}

    fractal = make_fractal(WIDTH, HEIGHT, coord_x, coord_y, depth, real, imaginary);

    //vytvoreni noveho vlakna pro vykresleni fraktalu
    int err = pthread_create(&drawing, NULL, thread_fractal, fractal);
    if (err != 0) {
        printf("ERROR\n");
        return 0;
	}
    pthread_join(drawing, NULL);

    }
    //vycisteni pameti
    free(fractal);

    return 0;
}


void *thread_fractal(void *args) {

    uint16_t *fractal = (uint16_t *) args;
    parlcd_write_cmd(parlcd_mem_base, 0x2c);
    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        parlcd_write_data(parlcd_mem_base, *(fractal + i));
    }

    free(fractal);

    return NULL;
}


uint16_t *
make_fractal(int width, int height, double coord_x, double coord_y, int iters,double c_real, double c_imag) {

	if (iters == 0){
		iters = START_DEPTH;
	}
	//definice barev
	static const uint16_t RED = 0b1111100000000000;
	static const uint16_t GREEN = 0b0000011111100000;
	static const uint16_t BLUE = 0b0000000000011111;
    double new_real, new_imag, old_real, old_imag;   //kazdou iteraci spocita nove hodnoty pro dany pixel

    uint16_t *final = (uint16_t *) calloc(width * height, sizeof(uint16_t)); // pole pro finalni obrazek
    unsigned int f_index = 0;

	//jede pres kazdy pixel
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
	//spocita realnou a imaginarni cast dle pozice tlacitek
            new_real = 1.5 * (x-width/2)/(0.5*width) + coord_x;
            new_imag = (y-height/2)/(0.5*height)+coord_y;

            int i;
            for (i = 0; i < iters; i++) {
                old_real = new_real;
                old_imag = new_imag;
		//spocita realnou a imaginarni
                new_real = old_real * old_real - old_imag * old_imag + c_real;
                new_imag = 2 * old_real * old_imag + c_imag;

                // jestli je pryc z obrazu -> stop
                if ((new_real * new_real + new_imag * new_imag) > 2) break;
            }

            // obarvime ho - lze libovolne kombinovat pro ruzne barvy
            double t = (double) i / (double) iters;
            uint8_t red = (uint8_t) (7*t *t*t* iters);
            uint8_t green = (uint8_t) (8 * (1 - t) *t * iters);
            uint8_t blue = (uint8_t) (6*(1 - t) * (1 - t) * t * iters);
            *(final + f_index++) = (((red) << 8) & RED) | (((green) << 3) & GREEN) | (((blue >> 3)) & BLUE);
        }
    }
    return final;
}

void make_animation(){
    uint32_t rgb_buts;
    //imaginarni a realne casti pro meneni setu
	double re_choose[] = {-0.4, -0.70176, -0.835, -0.8,  0.285, 0.285, 0.45};
	double im_choose[] = {0.6, -0.3842, 0.2321, 0.156,  0, 0.01, 0.1428};
	
	int index = 0;
	bool red_click = false;
 	mem_base = map_phys_address(SPILED_REG_BASE_PHYS, SPILED_REG_SIZE, 0); // tlacitka
 	if (mem_base == NULL) {
        printf("Error in membase\n");
        exit(1);
    }
    uint16_t *fractal;
	
	//meneni setu
	double real = re_choose[index];
    double imaginary = im_choose[index];
    
    pthread_t drawing; //vykreslovaci vlakno
	while (true){
		for (int i = 0; i < 6; i++){
			for (int j = 1; j <400; j++){
			    rgb_buts = *(volatile uint32_t *) (mem_base + SPILED_REG_KNOBS_8BIT_o); //hodnoty rgb
				red_click = (rgb_buts & 0xFF000000) >> 24 == 4;
				
				if(red_click){
					printf("KONEC\n");
					return;
				}
				index = (i +1)%6;
				real = re_choose[index];
				imaginary = im_choose[index];
				fractal = make_fractal(WIDTH, HEIGHT, 0, 0, real, imaginary, j);
				
				//vykreslovani vlakno
				int err = pthread_create(&drawing, NULL, thread_fractal, fractal);
				if (err != 0) {
					printf("ERROR\n");
					break;
				}
				
				//pro rychlejsi animaci
				if (j < 30){
					j += 3;
				}else if(j<130){
					j += 12;
				} else if(j < 300){
					j += 40;
				} else{
					j += 180;
				}
				
				pthread_join(drawing, NULL);
			}
			sleep(2);
		}
	}
	pthread_join(drawing, NULL);
	return;
}
