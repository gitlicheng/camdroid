#include <stdio.h>
#include <stdlib.h>
#include <zbar.h>

int zbar_dec(int width, int height, char* greyData, char* result, int result_len) {
    /* wrap image data */
    zbar_image_scanner_t *scanner = zbar_image_scanner_create();
	char* greyData2 = (char*)malloc(width*height);
	memcpy(greyData2, greyData, width*height);

    /* configure the reader */
    zbar_image_scanner_set_config(scanner, 0, ZBAR_CFG_ENABLE, 1);
    
    zbar_image_t *image = zbar_image_create();
    zbar_image_set_format(image, *(int*)"Y800");
    zbar_image_set_size(image, width, height);
    zbar_image_set_data(image, greyData2, width * height, zbar_image_free_data);

    /* scan the image for barcodes */
    int n = zbar_scan_image(scanner, image);
	int ret = 0;
	memset(result, 0, result_len);

    /* extract results */
    const zbar_symbol_t *symbol = zbar_image_first_symbol(image);
    for(; symbol; symbol = zbar_symbol_next(symbol)) {
        /* do something useful with results */
        zbar_symbol_type_t typ = zbar_symbol_get_type(symbol);
        const char *data = zbar_symbol_get_data(symbol);
		int data_len = strlen(data);
		if (data_len >= result_len) {
			break;
		}
		ret++;
		memcpy(result, data, data_len);
		result += data_len+1;
		result -= (data_len+1);
        printf("decoded %s symbol \"%s\"\n",
               zbar_get_symbol_name(typ), data);
    }

    /* clean up */
    zbar_image_destroy(image);
    zbar_image_scanner_destroy(scanner);

    return ret;
}
