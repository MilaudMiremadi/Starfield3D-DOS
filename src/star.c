#include <stdio.h>
#include <stdlib.h>
#include <dos.h>

#define PRGM_EXIT_NORM		0x00
#define PRGM_EXIT_ERROR		0x01

#define BIOS_VID_INTERRUPT	0x10
#define BIOS_SET_MODE		0x00
#define VGA_MODE_13h  		0x13
#define DOS_TEXT_MODE       0x03
#define INPUT_STATUS_1      0x03da
#define VRETRACE            0x08

#define SCREEN_WIDTH        320
#define SCREEN_HEIGHT       200
#define SCREEN_WIDTH_H      (SCREEN_WIDTH >> 1)
#define SCREEN_HEIGHT_H     (SCREEN_HEIGHT >> 1)
#define SCREEN_SIZE         (ushort16)(SCREEN_WIDTH * SCREEN_HEIGHT)
#define IDX(x, y)			((x) + (y) * 320)

#define STAR_COLOR			15
#define Z_MAX				128
#define NUM_STARS			512
#define TOTAL_FRAMES		128

#define CLEAR_COLOR			0
#define VIDEO_START_ADDR	0xa0000000
#define CLOCK_START_ADDR	0x0000046c

typedef unsigned char ubyte8;
typedef unsigned short ushort16;
typedef short short16;
typedef unsigned long uint32;
typedef long int32;

ubyte8 *video = (ubyte8 *) VIDEO_START_ADDR;
ushort16 *system_clock = (ushort16 *) CLOCK_START_ADDR;
ubyte8 *double_buffer;

typedef struct {
    short16 x, y, z;
} STAR;

/* Reset star to a random position */
void reset_star(STAR *star) {
    star->x = rand() % (SCREEN_WIDTH << 5) - ((SCREEN_WIDTH << 4));
    star->y = rand() % (SCREEN_HEIGHT << 5) - (SCREEN_HEIGHT << 4);
    star->z = rand() % Z_MAX;
}

/* Set DOS video mode */
void set_video_mode(ubyte8 mode) {
    union REGS regs;
    regs.h.ah = BIOS_SET_MODE;
    regs.h.al = mode;
    int86(BIOS_VID_INTERRUPT, &regs, &regs);
}

/* Clear screen to the palette color */
void clear(ubyte8 color) {
    memset(double_buffer, color, SCREEN_SIZE);
}

/* Blit double buffer to screen */
void blit() {
    while ((inp(INPUT_STATUS_1) & VRETRACE))
        ;
    while (!(inp(INPUT_STATUS_1) & VRETRACE))
        ;
    memcpy(video, double_buffer, SCREEN_SIZE);
}

/* Bounds checked pixel plotting */
void put_pix_checked(short16 x, short16 y, ubyte8 color) {
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) {
        return;
    }
    double_buffer[IDX(x, y)] = color;
}

int main() {
    register ushort16 i;
    register ushort16 frames;
    STAR *stars;
    STAR *curr;
    
    /* Allocate memory for the double buffer */
    if ((double_buffer = (ubyte8 *) malloc(SCREEN_SIZE)) == NULL) {
        printf("Not enough memory for double buffer\n");
        return PRGM_EXIT_ERROR;
    }
    
    /* Allocate memory for the stars */
    if ((stars = (STAR *) malloc(sizeof(STAR) * NUM_STARS)) == NULL) {
        printf("Not enough memory for stars\n");
        free(double_buffer);
        return PRGM_EXIT_ERROR;
    }
    
    /* Seed the pseudo random number generator */
    srand(*system_clock);
    /* Initialize stars */
    for (i = 0; i < NUM_STARS; i++) {
        reset_star(&stars[i]);
    }
    
    set_video_mode(VGA_MODE_13h);
    
    for (frames = 0; frames < TOTAL_FRAMES; frames++) {
        /* Clear screen */
        clear(CLEAR_COLOR);
        
        /* Draw stars */
        for (i = 0; i < NUM_STARS; i++) {
            curr = &stars[i];
            
            /* Near plane clip */
            if (curr->z > 0) {
                put_pix_checked(SCREEN_WIDTH_H + curr->x / curr->z, SCREEN_HEIGHT_H + curr->y / curr->z, STAR_COLOR);
            } else {
                /* Reset if behind near plane */
                reset_star(curr);
            }
            /* Move star closer to viewer */
            curr->z--;
        }
        
        /* Render frame */
        blit();
    }
    
    /* Cleanup */
    free(double_buffer);
    free(stars);
    
    set_video_mode(DOS_TEXT_MODE);
    return PRGM_EXIT_NORM;
}
