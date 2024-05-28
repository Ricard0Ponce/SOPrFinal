#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/fs.h>
#include <curses.h>
#include "ext4.h"
#include <string.h>

// Se declaran variables globales:
int aux = 1; // Iniciamos la bandera como 1.
int aux2 = 0;
bool activado = false; 

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
// Definimos las estructuras que se usaran:
struct ext4_super_block sb0;
struct ext4_group_desc groupDescriptor0;
struct ext4_inode inode0;
struct ext4_dir_entry_2 root;
struct ext4_dir_entry_2 segundoDirectorioAux;

// Se declaran las funciones:

// Función para obtener el tipo de partición como una cadena.
const char *get_partition_type(uint8_t type)
{
  // Se definen todos los tipos con su respectivo valor.
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

// Funcion que permite la lectura de un bloque
int readBlock(int fd, int currentLocation, void *structure, size_t size)
{
  if (lseek(fd, currentLocation, SEEK_SET) < 0)
  {
    perror("Error al buscar el bloque");
    return 1;
  }

  if (read(fd, structure, size) != size)
  {
    perror("Error al leer el bloque");
    return 1;
  }

  return 0; // Agregar este retorno al final
}

int getPosition(int value)
{
  return value * 0x400 + 0x100000;
}

// TODO: division con residuo
int getDirInode(int inode)
{
  // Calcular el descriptor de grupo para el segundo directorio
  int groupDescriptor = inode / 2040; // 2040 = 0x7F8, tamaño de un grupo
  // Se multiplica por 0x40 porque cada descriptor de grupo mide 0x40
  groupDescriptor *= 0x40;
  // Se suma 0x100800 porque es la posición donde comienzan los descriptores de
  // grupo
  groupDescriptor += 0x100800;

  return groupDescriptor;
}

// Imprimimos el super bloque.
void imprimeSuperBlock()
{
  mvprintw(0, 0, "\nComienza el super bloque de la particion 0: \n\n");
  mvprintw(3, 0, "Número de inodos en la partición 0: %u\n", sb0.s_inodes_count);
  mvprintw(5, 0, "Número de bloques en la partición 0: %u\n", sb0.s_blocks_count_lo);
  mvprintw(7, 0, "Etiqueta: %s\n", sb0.s_volume_name);
  mvprintw(9, 0, "Ultimo punto de montaje en la partición 0: %.64s\n", sb0.s_last_mounted);
  mvprintw(11, 0, "First Inode: %u\n", sb0.s_first_ino);
  mvprintw(13, 0, "Tamaño de bloque en la partición 0: %u\n", 1024 << sb0.s_log_block_size);
  // Calcular y mostrar el tamaño del inodo
  mvprintw(15, 0, "Tamaño del inodo en la partición 0: %u bytes\n", sb0.s_inode_size);
  // Calcular y mostrar los bloques por grupo
  mvprintw(17, 0, "Bloques por grupo en la partición 0: %u\n", sb0.s_blocks_per_group);
  mvprintw(19, 0, "Inodes por grupo: %u\n", sb0.s_inodes_per_group);
  mvprintw(21, 0, "Firma mágica en la partición 0: %x\n", sb0.s_magic);
  aux = aux + 1; // Aca indicamos que ya puede pasar a la siguiente parte pues el auxiliar ya es 2
  activado = false; 
}

// Imprimimos el descriptor de grupo.
void imprimeDescriptorDeGrupo()
{
  // Impresión de los datos del bloque de descriptores de grupo
  mvprintw(0, 0, "\n DESCRIPTOR DE GRUPOS \n");
  mvprintw(3, 0, "Directorios usados: %u\n", groupDescriptor0.bg_used_dirs_count_lo);
  mvprintw(5, 0, "Bitmap block: %u\n", groupDescriptor0.bg_block_bitmap_lo);
  mvprintw(7, 0, "Inode table: %u\n", groupDescriptor0.bg_inode_table_lo);
  aux2 = aux2 + 1;
  activado = false; 
}

// Imprimimos el directorio raiz.
void imprimeDirectorio()
{
  mvprintw(0, 0, "\n BLOQUE DEL DIRECTORIO ROOT \n");

  char *blockPtr = (char *)&root;
  int fila = 5; // Comenzar en la fila 5 para los nombres de los archivos

  while (blockPtr < (char *)&root + sizeof(root))
  {
    struct ext4_dir_entry_2 *entry = (struct ext4_dir_entry_2 *)blockPtr;

    // Validar que la entrada del directorio sea válida
    if (entry->inode != 0)
    {
      char filename[255]; // Buffer para el nombre del archivo
      // Copiar el nombre del archivo desde la entrada del directorio al buffer
      memcpy(filename, entry->name, entry->name_len);
      filename[entry->name_len] = '\0'; // Agregar el terminador nulo al final del nombre del archivo

      mvprintw(fila, 0, "Nombre: %s -> %u\n", filename, entry->inode);
      fila += 2; // Incrementar la fila en 2 para el próximo elemento
    }

    // Mover al siguiente bloque de directorio
    blockPtr += entry->rec_len;
    if (entry->rec_len == 0)
    {
      break; // Salir del bucle si el tamaño del registro es 0 para evitar un bucle infinito
    }
  }
  activado = false; 
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
  ////////ESTO ES DEL SUPER BLOQUE /////
  struct mbr_partition_entry entry; // Para suber bloque
  uint32_t sector_size = 512;       // Tamaño de sector típico para discos MBR
                                    // Leer la entrada de la primera partición (0)
  fseek(file, 0x1BE, SEEK_SET);
  fread(&entry, sizeof(entry), 1, file);
  uint64_t partition_size_bytes = entry.sectors * sector_size;
  double partition_size_mbi = (double)partition_size_bytes / 1048576;
  // Obtener superbloque de la partición 0
  uint32_t lba_start0 = entry.lba_start;
  int fd0 = fileno(file);

  // Buscar el superbloque en la partición 0
  if (lseek(fd0, (lba_start0 * sector_size) + 1024, SEEK_SET) < 0)
  {
    perror("Error al buscar el superbloque en la partición 0");
    fclose(file);
    return 1;
  }

  if (read(fd0, &sb0, sizeof(sb0)) != sizeof(sb0))
  {
    perror("Error al leer el superbloque en la partición 0");
    fclose(file);
    return 1;
  }

  /// ESTO ES PARA EL INICIO /////////////
  struct mbr_partition_entry entries[4];
  char chs_start[4][20], chs_end[4][20], partition_type[4][20];
  double partition_size_mb[4];

  // Leer la entrada de la primera partición (0)
  fseek(file, 0x1BE, SEEK_SET);
  fread(&entry, sizeof(entry), 1, file);

  if (lseek(fd0, (lba_start0 * sector_size) + 1024, SEEK_SET) < 0)
  {
    perror("Error al buscar el superbloque en la partición 0");
    fclose(file);
    return 1;
  }

  if (read(fd0, &sb0, sizeof(sb0)) != sizeof(sb0))
  {
    perror("Error al leer el superbloque en la partición 0");
    fclose(file);
    return 1;
  }

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
  int bandera = 0;

  do
  {
    if (bandera == 0)
    {
      for (int j = 0; j < 4; j++)
      {
        if (j == i)
        {
          attron(A_REVERSE);
        }
        mvprintw(4 + j, 2, "%4d  %15s      %-10s  %10s %12u  %8.2f", j, chs_start[j], partition_type[j], chs_end[j], entries[j].lba_start, partition_size_mb[j]);

        if (j == i)
        {
          attroff(A_REVERSE);
        }
      }
    }
    else if (bandera == 1 && aux != 3 && activado == true)
    {
      clear();
      imprimeSuperBlock(); // Actualizamos la vista al super bloque
    }
    if (aux == 3)
    {
      int currentLocation = (lba_start0 * sector_size) + 1024;
      int fd0 = open("imagen.img", O_RDONLY);
      if (fd0 < 0)
      {
        perror("Error al abrir el archivo");
        return 1;
      }

      currentLocation = currentLocation + 1024;
      // Leer el bloque de descriptores de grupo
      if (readBlock(fd0, currentLocation, &groupDescriptor0,
                    sizeof(groupDescriptor0)) != 0)
      {
        return 1;
      }

      int dirBlock = getPosition(inode0.i_block[5]);
      currentLocation = dirBlock;
      clear();
      imprimeDescriptorDeGrupo(); // Actualizamos la vista al descriptor de grupo
    }

    if (aux2 == 2)
    {
      clear();
      // Posición del primer inode
      int firstInode = getPosition(
          groupDescriptor0
              .bg_inode_table_lo); // obtener la posición del primer inode
      // Segundo inode (el root)
      int secondInode = firstInode + 0x100; // por cada inode se ocupan 0x100
      int currentLocation;
      currentLocation = secondInode; // Actualiza la localizacion.
      if (readBlock(fd0, currentLocation, &inode0, sizeof(inode0)) != 0)
      {
        return 1;
      }

      currentLocation = getPosition(inode0.i_block[5]);

      // Leer el bloque del directorio root
      if (readBlock(fd0, currentLocation, &root, sizeof(root)) != 0)
      {
        return 1;
      }
      bandera = 2;
      aux = aux + 1;
      imprimeDirectorio(); // Invocamos la funcion que imprime al directorio.
    }

    if (bandera == 0)
    {
      move(4 + i, 2);
      refresh();
    }

    c = leeChar();

    switch (c)
    {
    case 0x1B5B41: // Flecha arriba
      i = (i > 0) ? i - 1 : 3;
      break;
    case 0x1B5B42: // Flecha abajo
      i = (i < 3) ? i + 1 : 0;
      break;
    case 10: // Enter key
      bandera = 1;
      activado = true;
    default:
      break;
    }
  } while (c != 'q');
  endwin(); // Mueve la liberación de memoria aquí
  return 0;
}
