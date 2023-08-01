#include "bitmap.h"

#include <stdio.h>
#include <stdlib.h>

void _fskip(FILE *fp, int num_bytes)
{
   int i;
   for (i=0; i<num_bytes; i++)
      fgetc(fp);
}

void load_bmp_to_buffer(char *file, byte *screen_buffer, word buffer_width, word buffer_height, byte *palette, byte palette_offset)
{
    FILE *fp;
    dword index, x;
    word num_colors, width, height;

    /* open the file */
    if ((fp = fopen(file,"rb")) == NULL)
    {
        printf("Error opening file %s.\n",file);
        exit(1);
    }

    /* check to see if it is a valid bitmap file */
    if (fgetc(fp)!='B' || fgetc(fp)!='M')
    {
        fclose(fp);
        printf("%s is not a bitmap file.\n",file);
        exit(1);
    }

    /* read in the width and height of the image, and the
        number of colors used; ignore the rest */
    _fskip(fp,16);
    fread(&width, sizeof(word), 1, fp);
    _fskip(fp,2);
    fread(&height,sizeof(word), 1, fp);
    _fskip(fp,22);
    fread(&num_colors,sizeof(word), 1, fp);
    _fskip(fp,6);

    /* assume we are working with an 8-bit file */
    if (num_colors==0) num_colors=256;

    /* read the palette information */
    for(index = palette_offset; index < palette_offset + num_colors; index++)
    {
        palette[(int)(index*3+2)] = fgetc(fp) >> 2;
        palette[(int)(index*3+1)] = fgetc(fp) >> 2;
        palette[(int)(index*3+0)] = fgetc(fp) >> 2;
        x=fgetc(fp);
    }

    for(index = 0; index < buffer_height; index++) {
        for(x = 0; x < buffer_width; x++) {
            screen_buffer[(dword) buffer_width * ((dword) buffer_height - index - 1) + x] = (byte)fgetc(fp);
        }
    }

    fclose(fp);
}
