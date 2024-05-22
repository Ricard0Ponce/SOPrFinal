#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/fs.h>
#include <curses.h>

// Definición de la estructura de entrada de partición MBR
struct mbr_partition_entry
{
  uint8_t status;
  uint8_t chs_start[3];
  uint8_t type;
  uint8_t chs_end[3];
  uint32_t lba_start;
  uint32_t sectors;
};

// Función para obtener el tipo de partición como una cadena
const char *get_partition_type(uint8_t type)
{
  switch (type)
  {
  case 0x00:
    return "Empty";
  case 0x01:
    return "FAT12";
  case 0x04:
    return "FAT16 <32M";
  case 0x05:
    return "Extended";
  case 0x06:
    return "FAT16";
  case 0x07:
    return "NTFS";
  case 0x0B:
    return "FAT32";
  case 0x0C:
    return "FAT32 LBA";
  case 0x0E:
    return "FAT16 LBA";
  case 0x0F:
    return "Extended LBA";
  case 0x83:
    return "Linux";
  default:
    return "Unknown";
  }
}

// Función para imprimir la información CHS
void print_chs(uint8_t chs[3], char *buffer)
{
  uint16_t cylinder = ((chs[1] & 0xC0) << 2) | chs[2];
  uint8_t head = chs[0];
  uint8_t sector = chs[1] & 0x3F;
  sprintf(buffer, " %5u | %3u | %3u ", cylinder, head, sector);
}

// Función para leer la entrada de partición desde el archivo
int read_partition_entry(FILE *file, struct mbr_partition_entry *entry, int index)
{
  fseek(file, 0x1BE + index * sizeof(struct mbr_partition_entry), SEEK_SET);
  return fread(entry, sizeof(struct mbr_partition_entry), 1, file);
}

// Función para leer caracteres de entrada con ncurses
int leeChar()
{
  int chars[5];
  int ch, i = 0;
  nodelay(stdscr, TRUE);
  while ((ch = getch()) == ERR)
    ; /* Espera activa */
  ungetch(ch);
  while ((ch = getch()) != ERR)
  {
    chars[i++] = ch;
  }
  int res = 0;
  for (int j = 0; j < i; j++)
  {
    res <<= 8;
    res |= chars[j];
  }
  return res;
}

int main()
{
  FILE *file = fopen("imagen.img", "rb");
  if (!file)
  {
    perror("Error al abrir la imagen");
    return 1;
  }

  struct mbr_partition_entry entries[4];
  char chs_start[4][20], chs_end[4][20], partition_type[4][20];
  double partition_size_mb[4];
  uint32_t sector_size = 512;

  for (int i = 0; i < 4; i++)
  {
    if (!read_partition_entry(file, &entries[i], i))
    {
      fclose(file);
      perror("Error al leer la entrada de partición");
      return 1;
    }
    partition_size_mb[i] = (double)(entries[i].sectors * sector_size) / 1048576;
    print_chs(entries[i].chs_start, chs_start[i]);
    print_chs(entries[i].chs_end, chs_end[i]);
    snprintf(partition_type[i], 20, "%s", get_partition_type(entries[i].type));
  }
  fclose(file);

  initscr();
  raw();
  noecho();
  cbreak();

  attron(A_BOLD);
  mvprintw(2, 2, "Part.        CHS inicio        Tipo              CHS fin       LBA inicio    TAM (MB)");
  attroff(A_BOLD);

  int i = 0;
  int c;
  do
  {
    for (int j = 0; j < 4; j++)
    {
      if (j == i)
      {
        attron(A_REVERSE);
      }
      /*mvprintw(4 + j, 2, "%4d  %17s     %-20s  %17s  %12u  %8.2f", j, chs_start[j], partition_type[j], chs_end[j], entries[j].lba_start, partition_size_mb[j]);*/
      mvprintw(4 + j, 2, "%4d  %15s      %-10s  %10s %12u  %8.2f", j, chs_start[j], partition_type[j], chs_end[j], entries[j].lba_start, partition_size_mb[j]);

      if (j == i)
      {
        attroff(A_REVERSE);
      }
    }
    move(4 + i, 2);
    refresh();
    c = leeChar();
    switch (c)
    {
    case 0x1B5B41: /* Flecha arriba */
      i = (i > 0) ? i - 1 : 3;
      break;
    case 0x1B5B42: /* Flecha abajo */
      i = (i < 3) ? i + 1 : 0;
      break;
    default:
      break;
    }
  } while (c != 'q');
  endwin();
  return 0;
}
