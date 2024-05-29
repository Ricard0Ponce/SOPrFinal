#include <stdio.h>      // Librería estándar de entrada y salida
#include <stdint.h>     // Definiciones de tipos de datos enteros con tamaño fijo
#include <stdlib.h>     // Librería estándar de funciones generales
#include <fcntl.h>      // Definiciones de control de archivos
#include <unistd.h>     // Definiciones de constantes y tipos estándar, y funciones POSIX
#include <sys/types.h>  // Definiciones de tipos de datos
#include <sys/stat.h>   // Definiciones de estructura de estado y modos de archivo
#include <linux/fs.h>   // Definiciones específicas del sistema de archivos de Linux
#include <curses.h>     // Librería para manipulación de pantalla y entrada de teclado
#include "ext4.h"       // Definiciones específicas del sistema de archivos ext4
#include <string.h>     // Funciones para manipulación de cadenas de caracteres

// Se declaran variables globales:
int aux = 1;             // Bandera de control inicializada en 1
int aux2 = 0;            // Segunda bandera de control inicializada en 0
bool activado = false;   // Estado booleano inicializado en falso
int mover=3,moverC=3;
bool restringido=false;
int ind=0,ind2=0;
int residuo =0;
// Estructura para almacenar inodos y nombres
struct inode_name {
    uint32_t inode;
    char name[255];
};

// Arreglo para almacenar inodos y nombres
struct inode_name inode_names[255];
struct inode_name inode_names2[255];
int inode_count = 0;  // Contador de inodos
int inode_count2=0;
uint32_t sector_size = 512;

// Definición de la estructura de entrada de partición MBR
struct mbr_partition_entry {
    uint8_t status;       // Estado de la partición
    uint8_t chs_start[3]; // Dirección CHS de inicio
    uint8_t type;         // Tipo de partición
    uint8_t chs_end[3];   // Dirección CHS de fin
    uint32_t lba_start;   // Dirección de inicio en LBA
    uint32_t sectors;     // Número de sectores en la partición
};

// Definimos las estructuras que se usarán:
struct ext4_super_block sb0;              // Estructura del super bloque de ext4
struct ext4_group_desc groupDescriptor0;  // Estructura del descriptor de grupo de ext4
struct ext4_inode inode0;                 // Estructura del inodo de ext4
struct ext4_dir_entry_2 root;             // Estructura del directorio raíz de ext4
struct ext4_dir_entry_2 segundoDirectorioAux; // Estructura auxiliar para el segundo directorio

// Función para obtener el tipo de partición como una cadena.
const char *get_partition_type(uint8_t type) {
    // Se definen todos los tipos de partición con su respectivo valor.
    switch (type) {
    case 0x00: return "Empty";
    case 0x01: return "FAT12";
    case 0x04: return "FAT16 <32M";
    case 0x05: return "Extended";
    case 0x06: return "FAT16";
    case 0x07: return "NTFS";
    case 0x0B: return "FAT32";
    case 0x0C: return "FAT32 LBA";
    case 0x0E: return "FAT16 LBA";
    case 0x0F: return "Extended LBA";
    case 0x83: return "Linux";
    default: return "Unknown";
    }
}

// Función que permite la lectura de un bloque desde el archivo de disco
int readBlock(int fd, int currentLocation, void *structure, size_t size) {
    // Mueve el puntero del archivo a la ubicación del bloque
    if (lseek(fd, currentLocation, SEEK_SET) < 0) {
        perror("Error al buscar el bloque");
        return 1;
    }

    // Lee el bloque en la estructura proporcionada
    if (read(fd, structure, size) != size) {
        perror("Error al leer el bloque");
        return 1;
    }

    return 0; // Retorna 0 si la operación fue exitosa
}

// Función que calcula la posición de un bloque en el disco
int getPosition(int value) {
    return value * 0x400 + 0x100000;
}

// TODO: división con residuo
// Función que calcula la posición del descriptor de grupo para un inodo
int getDirInode(int inode) {
    // Calcular el descriptor de grupo para el segundo directorio
    int groupDescriptor = inode / 2040; // 2040 = 0x7F8, tamaño de un grupo
    residuo = inode % 2040;
    // Se multiplica por 0x40 porque cada descriptor de grupo mide 0x40
    groupDescriptor *= 0x40;
    // Se suma 0x100800 porque es la posición donde comienzan los descriptores de grupo
    groupDescriptor += 0x100800;

    return groupDescriptor;
}
// Imprimimos el super bloque.
void imprimeSuperBlock()
{
  mvprintw(0, 0, "\nComienza el super bloque de la particion 0: \n\n");
  mvprintw(3, 0, "Número de inodos en la partición 0: %u\n", sb0.s_inodes_count);  // Número total de inodos
  mvprintw(5, 0, "Número de bloques en la partición 0: %u\n", sb0.s_blocks_count_lo); // Número total de bloques
  mvprintw(7, 0, "Etiqueta: %s\n", sb0.s_volume_name);  // Nombre del volumen
  mvprintw(9, 0, "Ultimo punto de montaje en la partición 0: %.64s\n", sb0.s_last_mounted);  // Último punto de montaje
  mvprintw(11, 0, "First Inode: %u\n", sb0.s_first_ino);  // Primer inodo
  mvprintw(13, 0, "Tamaño de bloque en la partición 0: %u\n", 1024 << sb0.s_log_block_size);  // Tamaño del bloque
  mvprintw(15, 0, "Tamaño del inodo en la partición 0: %u bytes\n", sb0.s_inode_size);  // Tamaño del inodo
  mvprintw(17, 0, "Bloques por grupo en la partición 0: %u\n", sb0.s_blocks_per_group);  // Bloques por grupo
  mvprintw(19, 0, "Inodes por grupo: %u\n", sb0.s_inodes_per_group);  // Inodos por grupo
  mvprintw(21, 0, "Firma mágica en la partición 0: %x\n", sb0.s_magic);  // Firma mágica
  aux = aux + 1;  // Actualización de la bandera de control
  activado = false;  // Desactivar la bandera
}

// Imprimimos el descriptor de grupo.
void imprimeDescriptorDeGrupo()
{
  mvprintw(0, 0, "\n DESCRIPTOR DE GRUPOS \n");
  mvprintw(3, 0, "Directorios usados: %u\n", groupDescriptor0.bg_used_dirs_count_lo);  // Directorios usados
  mvprintw(5, 0, "Bitmap block: %u\n", groupDescriptor0.bg_block_bitmap_lo);  // Bloque de bitmap
  mvprintw(7, 0, "Inode table: %u\n", groupDescriptor0.bg_inode_table_lo);  // Tabla de inodos
  aux2 = aux2 + 1;  // Actualización de la segunda bandera de control
  activado = false;  // Desactivar la bandera
}

// Imprimimos el directorio raiz.
void imprimeDirectorio()
{
  mvprintw(0, 0, "\n BLOQUE DEL DIRECTORIO ROOT \n");

  char *blockPtr = (char *)&root;  // Puntero al bloque del directorio raíz
  int fila = 3;  // Comenzar en la fila 5 para los nombres de los archivos

  while (blockPtr < (char *)&root + sizeof(root))
  {
    struct ext4_dir_entry_2 *entry = (struct ext4_dir_entry_2 *)blockPtr;

    // Validar que la entrada del directorio sea válida
    if (entry->inode != 0)
    {
      char filename[255];  // Buffer para el nombre del archivo
      memcpy(filename, entry->name, entry->name_len);  // Copiar el nombre del archivo al buffer
      filename[entry->name_len] = '\0';  // Agregar el terminador nulo al final del nombre del archivo
   // Almacenar el inodo y el nombre en el arreglo global
      inode_names[inode_count].inode = entry->inode;
      strncpy(inode_names[inode_count].name, filename, sizeof(inode_names[inode_count].name) - 1);
      inode_names[inode_count].name[sizeof(inode_names[inode_count].name) - 1] = '\0';
      inode_count++;

      mvprintw(fila, 0, "Nombre: %s -> %u\n", filename, entry->inode);  // Mostrar el nombre del archivo y el número de inodo
      fila += 2;  // Incrementar la fila en 2 para el próximo elemento
    }

    // Mover al siguiente bloque de directorio
    blockPtr += entry->rec_len;
    if (entry->rec_len == 0)
    {
      break;  // Salir del bucle si el tamaño del registro es 0 para evitar un bucle infinito
    }
  }
  activado = false;  // Desactivar la bandera
}

void imprimeCarpeta(int inode,int fd0,FILE *file){

char *blockPtr = (char *)&root;
  while (blockPtr < (char *)&root + sizeof(root)) {
    struct ext4_dir_entry_2 *entry = (struct ext4_dir_entry_2 *)blockPtr;

    // Asigno inode
    entry->inode = inode;
    segundoDirectorioAux = *entry;
    // Move to the next directory entry
    blockPtr += entry->rec_len;
  }
  int calculoDescriptorDeGrupo = getDirInode(segundoDirectorioAux.inode);
  int currentLocation = calculoDescriptorDeGrupo;

    struct ext4_group_desc groupDescriptor1;
     if (readBlock(fd0, currentLocation, &groupDescriptor1,
                sizeof(groupDescriptor1)) != 0) {
    fclose(file);
  }

  currentLocation = getPosition(groupDescriptor1.bg_inode_table_lo);
  currentLocation += ((residuo - 1) * 0x100);

    struct ext4_inode inode1;
  if (readBlock(fd0, currentLocation, &inode1, sizeof(inode1)) != 0) {
    fclose(file);
  }
  int dirBlock = getPosition(inode1.i_block[5]);
  currentLocation = dirBlock;
  
  struct ext4_dir_entry_2 directorio;

  // Leer el bloque del segundo directorio
  if (readBlock(fd0, currentLocation, &directorio, sizeof(directorio)) != 0) {
    fclose(file);
  }
  mvprintw(1,5,"BLOQUE DEL DIRECTORIO");

  char *blockPtrD = (char *)&directorio;
  int FilaC=3;
  while (blockPtrD < (char *)&directorio + sizeof(directorio)) {
    struct ext4_dir_entry_2 *entry = (struct ext4_dir_entry_2 *)blockPtrD;

      char filename[255];  // Buffer para el nombre del archivo
      memcpy(filename, entry->name, entry->name_len);  // Copiar el nombre del archivo al buffer
      filename[entry->name_len] = '\0';  // Agregar el terminador nulo al final del nombre del archivo
   // Almacenar el inodo y el nombre en el arreglo global
      inode_names2[inode_count2].inode = entry->inode;
      strncpy(inode_names2[inode_count2].name, filename, sizeof(inode_names2[inode_count2].name) - 1);
      inode_names2[inode_count2].name[sizeof(inode_names2[inode_count2].name) - 1] = '\0';
      inode_count2++;
    mvprintw(FilaC,0,"Nombre: %s -> %u\n", entry->name, entry->inode);
    FilaC++;

    // Move to the next directory entry
    blockPtrD += entry->rec_len;
  }

}



// Función para imprimir la información CHS
void print_chs(uint8_t chs[3], char *buffer)
{
  uint16_t cylinder = ((chs[1] & 0xC0) << 2) | chs[2];  // Calcular el cilindro
  uint8_t head = chs[0];  // Cabeza de lectura/escritura
  uint8_t sector = chs[1] & 0x3F;  // Sector
  sprintf(buffer, " %5u | %3u | %3u ", cylinder, head, sector);  // Formatear y almacenar la información en el buffer
}

// Función para leer la entrada de partición desde el archivo
int read_partition_entry(FILE *file, struct mbr_partition_entry *entry, int index)
{
  fseek(file, 0x1BE + index * sizeof(struct mbr_partition_entry), SEEK_SET);  // Mover el puntero del archivo a la entrada de partición deseada
  return fread(entry, sizeof(struct mbr_partition_entry), 1, file);  // Leer la entrada de partición
}

// Función para leer caracteres de entrada con ncurses
int leeChar()
{
  int chars[5];  // Buffer para almacenar los caracteres
  int ch, i = 0;
  nodelay(stdscr, TRUE);  // No esperar por la entrada del usuario
  while ((ch = getch()) == ERR);  // Espera activa
  ungetch(ch);  // Volver a poner el carácter en la cola de entrada
  while ((ch = getch()) != ERR)
  {
    chars[i++] = ch;  // Almacenar el carácter leído
  }
  int res = 0;
  for (int j = 0; j < i; j++)
  {
    res <<= 8;
    res |= chars[j];  // Construir el resultado final a partir de los caracteres leídos
  }
  return res;
}


int main()
{
  // Abre el archivo de imagen "imagen.img" en modo lectura binaria
  FILE *file = fopen("imagen.img", "rb");
  if (!file)
  {
    // Si hay un error al abrir el archivo, muestra un mensaje de error y sale del programa con código de error 1
    perror("Error al abrir la imagen");
    return 1;
  }

  // Estructura para almacenar la entrada de la tabla de particiones MBR
  struct mbr_partition_entry entry;
  // Tamaño de sector típico para discos MBR


  // Lee la entrada de la primera partición (índice 0) de la tabla de particiones MBR
  fseek(file, 0x1BE, SEEK_SET);
  fread(&entry, sizeof(entry), 1, file);
  // Calcula el tamaño de la partición en bytes y en megabytes
  uint64_t partition_size_bytes = entry.sectors * sector_size;
  double partition_size_mbi = (double)partition_size_bytes / 1048576;

  // Obtiene el número de sector de inicio de la partición 0
  uint32_t lba_start0 = entry.lba_start;
  // Obtiene el descriptor de archivo correspondiente al archivo abierto
  int fd0 = fileno(file);

  // Busca el superbloque en la partición 0
  if (lseek(fd0, (lba_start0 * sector_size) + 1024, SEEK_SET) < 0)
  {
    // Si hay un error, muestra un mensaje de error, cierra el archivo y sale del programa con código de error 1
    perror("Error al buscar el superbloque en la partición 0");
    fclose(file);
    return 1;
  }

  // Lee el superbloque de la partición 0
  if (read(fd0, &sb0, sizeof(sb0)) != sizeof(sb0))
  {
    // Si hay un error, muestra un mensaje de error, cierra el archivo y sale del programa con código de error 1
    perror("Error al leer el superbloque en la partición 0");
    fclose(file);
    return 1;
  }

  // Lee nuevamente la entrada de la primera partición para reiniciar la lectura
  fseek(file, 0x1BE, SEEK_SET);
  fread(&entry, sizeof(entry), 1, file);

  // Busca el superbloque nuevamente para reiniciar la búsqueda
  if (lseek(fd0, (lba_start0 * sector_size) + 1024, SEEK_SET) < 0)
  {
    perror("Error al buscar el superbloque en la partición 0");
    fclose(file);
    return 1;
  }

  // Lee el superbloque nuevamente
  if (read(fd0, &sb0, sizeof(sb0)) != sizeof(sb0))
  {
    perror("Error al leer el superbloque en la partición 0");
    fclose(file);
    return 1;
  }

  // Arreglo de estructuras para almacenar las entradas de las particiones
  struct mbr_partition_entry entries[4];
  // Arreglos para almacenar información sobre las particiones
  char chs_start[4][20], chs_end[4][20], partition_type[4][20];
  double partition_size_mb[4];

  // Lee las entradas de las particiones y calcula el tamaño de cada partición en megabytes
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

  // Inicializa la pantalla de la interfaz gráfica
  initscr();
  raw();
  noecho();
  cbreak();

  // Imprime el encabezado de la tabla de particiones en la pantalla
  attron(A_BOLD);
  mvprintw(2, 2, "Part.        CHS inicio        Tipo              CHS fin       LBA inicio    TAM (MB)");
  attroff(A_BOLD);

  // Variables para controlar la posición y el movimiento en la tabla de particiones
  int i = 0;
  int c;
  int bandera = 0;

  // Bucle principal que maneja la interfaz gráfica
  do
  {
    // Imprime la tabla de particiones en la pantalla
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
      // Borra la pantalla y actualiza la vista al superbloque
      clear();
      imprimeSuperBlock();
    }
    if (aux == 3)
    {
      // Calcula la ubicación actual
      int currentLocation = (lba_start0 * sector_size) + 1024;
      // Abre el archivo de imagen en modo solo lectura
      int fd0 = open("imagen.img", O_RDONLY);
      if (fd0 < 0)
      {
        perror("Error al abrir el archivo");
        return 1;
      }

      currentLocation = currentLocation + 1024;
      // Lee el bloque de descriptores de grupo
      if (readBlock(fd0, currentLocation, &groupDescriptor0,
                    sizeof(groupDescriptor0)) != 0)
      {
        return 1;
      }

      // Calcula la posición del bloque de directorio
      int dirBlock = getPosition(inode0.i_block[5]);
      currentLocation = dirBlock;
      // Borra la pantalla y actualiza la vista al descriptor de grupo
      clear();
      imprimeDescriptorDeGrupo();
    }

    if (aux2 == 2)
    {
      int largoLinea=20;
      // Borra la pantalla
      clear();
      // Obtiene la posición del primer inode
      int firstInode = getPosition(
          groupDescriptor0
              .bg_inode_table_lo);
      // Obtiene la posición del segundo inode (el root)
      int secondInode = firstInode + 0x100;
      int currentLocation;
      currentLocation = secondInode;
      if (readBlock(fd0, currentLocation, &inode0, sizeof(inode0)) != 0)
      {
        return 1;
      }

      currentLocation = getPosition(inode0.i_block[5]);

      // Lee el bloque del directorio root
      if (readBlock(fd0, currentLocation, &root, sizeof(root)) != 0)
      {
        return 1;
      }
      bandera = 2;
      aux = aux + 1;
      // Invoca la función que imprime al directorio
      imprimeDirectorio();
      move(mover, 0);
     // Iluminar toda la línea en donde se encuentra el cursor de desplazamiento
      attron(COLOR_PAIR(1) | A_REVERSE);  // Activar el par de colores 1 y el modo de inversión de color
      mvhline(mover, 0, ' ', largoLinea);  // Dibujar una línea horizontal de espacios
      mvprintw(mover, 0, "Nombre: %s -> %u", inode_names[ind].name, inode_names[ind].inode);
      attroff(COLOR_PAIR(1) | A_REVERSE); 
    }

    if(aux2==3){
      int largoLinea=20;
      clear();
      int inode = inode_names[ind].inode;
      imprimeCarpeta(inode,fd0,file);
      move(moverC, 0);
      attron(COLOR_PAIR(1) | A_REVERSE);  // Activar el par de colores 1 y el modo de inversión de color
      mvhline(moverC, 0, ' ', largoLinea);  // Dibujar una línea horizontal de espacios
      mvprintw(moverC, 0, "Nombre: %s -> %u", inode_names2[ind2].name, inode_names2[ind2].inode);
      attroff(COLOR_PAIR(1) | A_REVERSE); 
      mvprintw(15, 0, "I: %d ", ind2);
      mvprintw(16, 0, "Move: %d ", moverC);
    }

    if (bandera == 0)
    {
      // Mueve el cursor a la posición actual en la tabla de particiones
      move(4 + i, 2);
      refresh();
    }

    // Lee la tecla presionada por el usuario
    c = leeChar();

    // Maneja el movimiento en la tabla de particiones según la tecla presionada
    switch (c)
    {
    case 0x1B5B41: // Flecha arriba
      i = (i > 0) ? i - 1 : 3;
      if(aux2==2 && mover>3){
        mover = mover-2;
        ind--;
      }
      if(aux2==3 && moverC>3 && ind2>=0){
        moverC --;
        ind2--;
      }
      //activado=true;
      break;
    case 0x1B5B42: // Flecha abajo
      i = (i < 3) ? i + 1 : 0;
      if(mover<10){
      mover = mover+2;
      if(aux2==2)
      ind++;
      }

      if(aux2==3 && moverC<13){
      moverC = moverC+1;
        ind2++;
        }
      break;
    case 10: // Enter key
      bandera = 1;
      activado = true;
      if(aux2==2){
      clear();
      aux2=aux2+1;
      }
    default:
      break;
    }
  } while (c != 'q');

  // Finaliza la interfaz gráfica y retorna 0 para indicar que el programa finalizó sin errores
  endwin();
  return 0;
}
