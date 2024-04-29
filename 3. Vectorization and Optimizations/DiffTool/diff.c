#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h>
#include <io_png.h>
#include <math.h>

int main ( int argc, char **argv )
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input> <output>\n", argv[0]);
        return EXIT_FAILURE;
    }

    size_t aw, ah;
    uint8_t *a = read_png_u8_rgb ( argv[1], &aw, &ah );

    if( a == NULL) {
        fprintf(stderr,"Failed to open image: %s\n", argv[1]);
        return EXIT_FAILURE;
    }
    size_t  bw, bh;
    uint8_t *b = read_png_u8_rgb ( argv[2], &bw, &bh );
    if( b == NULL) {
        fprintf(stderr,"Failed to open image: %s\n", argv[2]);
        free(a);
        return EXIT_FAILURE;
    }

    if ( ! (aw == bw && ah == bh ) ){
        fprintf(stderr, "Images are not equal in size, so no point in checking if they are the same.\n");
        free(a);
        free(b);
        return EXIT_FAILURE;
    }

    uint64_t difference = 0;
    uint64_t pixels = 0;
    uint64_t max_diff_pixel = 0;
	for ( size_t index = 0; index < (ah*aw); index++){
		uint64_t dabs = 0;
		if( a[index] == b[index]) continue;
		else if ( a[index] > b[index] ) dabs = a[index]-b[index];
		else dabs = b[index]-a[index];
		difference += dabs;
		if(dabs) {
			pixels++;
			max_diff_pixel = (max_diff_pixel < (uint64_t)dabs)? (uint64_t)dabs:max_diff_pixel;
		}

	}
    if ( difference != 0){
    fprintf(stderr, "Found %llu difference in %llu pixels, max distance: %llu\n", difference, pixels, max_diff_pixel);
    }

    free(a);
    free(b);
    return difference == 0 ? EXIT_SUCCESS: EXIT_FAILURE;
}
