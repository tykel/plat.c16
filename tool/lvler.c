#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#define expect_eq(x,y,s) do{if((x)!=(y)){fprintf(stderr,"test failed: %s. Expected %d, got %d\n",(s),(x),(y));}else{printf("test passed: %s\n",(s));}}while(0);
#define expect_eqmem(x,y,s) do{if(memcmp((x),(y),sizeof((x)))){fprintf(stderr,"test failed: %s\n",s);}else{printf("test passed: %s\n",(s));}}while(0);
#define MIN(x,y) ((x) < (y) ? (x) : (y))

char line[256];

struct metadata {
   int width;
   int height;
   char name[256];
} meta;

struct tile {
   int set;
   int frames;
} tiles[256];

uint16_t *map;
int mapy = 0;

int (*readln_fptr)(FILE *, char *);

void print_buffer(uint8_t *buf, size_t len)
{
   uint8_t *p = buf;
   uint8_t *end = buf + len;
   while (p < end) {
      printf("%02x ", *p++);
   }
   printf("\n");
}

int readln_metadata(FILE *f, char *line)
{
   int len = 256;
   fgets(line, 256, f);
   if (line[0] == '.') {
      readln_fptr = NULL;
      return 0;
   }
   // Parse directives
   len = MIN(strnlen(line, 5), 256);
   if (!strncmp(line, "width", len)) {
      sscanf(line, "width %d\n", &meta.width);
   }
   len = MIN(strnlen(line, 6), 256);
   if (!strncmp(line, "height", len)) {
      sscanf(line, "height%d\n", &meta.height);
   }
   len = MIN(strnlen(line, 4), 256);
   if (!strncmp(line, "name", len)) {
      sscanf(line, "name %s\n", meta.name);
   }

   if (meta.width > 0 && meta.height > 0) {
      map = calloc(1, 2 * meta.width * meta.height);
   }
   return 0;
}

int readln_tiles(FILE *f, char *line)
{
   int tile_index;
   int tile_frames;
   int len = 256;
   fgets(line, 256, f);
   /* Handle new sections and empty lines. */
   len = MIN(strnlen(line, 5), 256);
   if (line[0] == '.') {
      readln_fptr = NULL;
      return 0;
   }
   if (line[0] == '\n') {
      return 0;
   }
   /* Parse tile entry. */
   sscanf(line, "%d %*s %d\n", &tile_index, &tile_frames);
   tiles[tile_index].set = 1;
   tiles[tile_index].frames = tile_frames;
   return 0;
}

int readln_map(FILE *f, char *line)
{
   char *tile;
   int mapx = 0;
   int len = 256;
   fgets(line, 256, f);
   /* Handle new sections and empty lines. */
   len = MIN(strnlen(line, 5), 256);
   if (line[0] == '.') {
      readln_fptr = NULL;
      return 0;
   }
   if (line[0] == '\n') {
      mapy += 1;
      return 0;
   }
   if (feof(f)) {
      return 0;
   }

   /* Parse map entries. */
   tile = strtok(line, " ");
   while (tile != NULL) {
      uint8_t tile_index = atoi(tile) & 0x7f;
      uint8_t tile_frames = (tiles[tile_index].frames - 1) & 1;
      uint tile_in_map = (tile_index + 1) | (tile_frames << 6);
      if (!strncmp(tile, "_", 1)) {
         map[mapy * meta.width + mapx] = 0;
      } else {
         map[mapy * meta.width + mapx] = tile_in_map;
      }
      mapx += 1;
      tile = strtok(NULL , " ");
   }

   mapy += 1;
   return 0;
}

int readln_solid(FILE *f, char *line)
{
   char *tile;
   int mapx = 0;
   int len = 256;
   fgets(line, 256, f);
   /* Handle new sections and empty lines. */
   len = MIN(strnlen(line, 5), 256);
   if (line[0] == '.') {
      readln_fptr = NULL;
      return 0;
   }
   if (line[0] == '\n') {
      mapy += 1;
      return 0;
   }
   if (feof(f)) {
      return 0;
   }

   /* Parse map entries. */
   tile = strtok(line, " ");
   while (tile != NULL) {
      uint8_t tile_solid = atoi(tile) & 1;
      uint tile_in_map = map[mapy * meta.width + mapx] | (tile_solid << 7);
      map[mapy * meta.width + mapx] = tile_in_map;
      mapx += 1;
      tile = strtok(NULL , " ");
   }

   mapy += 1;

   return 0;
}

int readln(FILE *f, char *line)
{
   if (readln_fptr != NULL) {
      return readln_fptr(f, line);
   }
   if (!strncmp(line, ".meta\n", 256)) {
      readln_fptr = readln_metadata;
      //printf("found metadata entry\n");
   } else if (!strncmp(line, ".tiles\n", 256)) {
      readln_fptr = readln_tiles;
      //printf("found tiles entry\n");
   } else if (!strncmp(line, ".map\n", 256)) {
      readln_fptr = readln_map;
      //printf("found map entry\n");
      mapy = 0;
      if (meta.width < 1 || meta.height < 1) {
         fprintf(stderr, "error: no width and/or height set in .meta section\n");
         return 1;
      }
   } else if (!strncmp(line, ".solid\n", 256)) {
      readln_fptr = readln_solid;
      //printf("found solid-ness entry\n");
      mapy = 0;
      if (meta.width < 1 || meta.height < 1) {
         fprintf(stderr, "error: no width and/or height set in .meta section\n");
         return 1;
      }
   }
   return 0;
}

size_t rle(uint8_t *src, uint8_t *dst, size_t len)
{
   uint8_t byte_to_rep = *src;
   uint8_t byte_count = 0;
   uint8_t *s = src;
   uint8_t *d = dst;

   while (s < src + len) {
      if (*s == byte_to_rep && byte_count < 0xff) {
         ++byte_count;
      } else {
         *d++ = byte_to_rep;
         *d++ = byte_count;
         byte_to_rep = *s;
         byte_count = 1;
      }
      ++s;
   }
   *d++ = byte_to_rep;
   *d++ = byte_count;
   return d - dst;
}

size_t compress(uint16_t *map, int cols, int rows, uint8_t **dst)
{
   const size_t bufsz = rows * cols;
   uint8_t *tmp_tiles = calloc(1, bufsz);
   uint8_t *rle_tiles = calloc(1, 2 * bufsz);
   uint8_t *rle_map;
   uint8_t *tmp;
   int x, y;
   int tiles_rle_size = 0;
   int total_rle_size = 0;

   for (y = 0; y < rows; y++) {
      for (x = 0; x < cols; x++) {
         uint16_t val = map[(y * cols + x)];
         tmp_tiles[y * cols + x] = val & 0xff;//0x7f;
      }
   }
   
   tiles_rle_size = rle(tmp_tiles, rle_tiles, bufsz);
#ifdef DEBUG
   printf("Raw tiles data:\n");
   print_buffer(tmp_tiles, bufsz);
   printf("RLE tiles data:\n");
   print_buffer(rle_tiles, tiles_rle_size);
   printf("RLE size ratio for tiles data: % 3.2f %%\n", 100.f*(float)tiles_rle_size/(float)bufsz);
#endif

   total_rle_size = tiles_rle_size + 2;
   printf("Raw map size: %d bytes\n", meta.width * meta.height * 2);
   printf("RLE map size: %d bytes\n", total_rle_size);
   printf("RLE size is % .2f %% of raw\n", 100.f*(float)total_rle_size/(float)(meta.width * meta.height * 2));

   /* Allow for the section sizes too! */
   rle_map = malloc(total_rle_size);
   tmp = rle_map;
   *(int16_t *)tmp = tiles_rle_size;
   tmp += 2;
   memcpy(tmp, rle_tiles, tiles_rle_size);

   *dst = rle_map;
   
   free(tmp_tiles);
   free(rle_tiles);

   return total_rle_size;
}

int main(int argc, char **argv)
{
   FILE *file_input;
   FILE *file_output;

   if (argc < 2 || !strncmp(argv[1], "-h", 2) || !strncmp(argv[1], "--help", 6)) {
      printf("usage: lvler <file.txt>\n");
   }

   if (!strncmp(argv[1], "test", 4)) {
      {
         uint8_t src[] = { 0, 0, 0, 0, 0, 1, 1, 0 };
         uint8_t expected_dst[] = { 0, 5, 1, 2, 0, 1 };
         uint8_t actual_dst[256] = { 0 };
         size_t len = rle(src, actual_dst, sizeof(src));
         expect_eq(sizeof(expected_dst), len, "RLE test (size)");
         expect_eqmem(expected_dst, actual_dst, "RLE test (contents)"); 
      }
      return 0;
   }

   readln_fptr = readln_metadata;

   file_input = fopen(argv[1], "r");
   while (!feof(file_input)) {
      readln(file_input, line);
   }
   fclose(file_input);

   if (meta.name == NULL) {
      fprintf(stderr, "error: no level name set in .meta section\n");
      return 1;
   }

   file_output = fopen(meta.name, "wb");
   
   if (argc > 2 && !strncmp(argv[2], "--rle", 5)) {
      uint8_t *rle_map;
      size_t len = compress(map, meta.width, meta.height, &rle_map);
      fwrite(rle_map, len, 1, file_output);
      free(rle_map);
   } else {
      fwrite(map, 2 * meta.width * meta.height, 1, file_output);
   }
   fclose(file_output);

   free(map);

   return 0;
}
