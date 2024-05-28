#include "ext4.h"
#include <fcntl.h>
#include <linux/fs.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

struct mbr_partition_entry {
  uint8_t status;
  uint8_t chs_start[3];
  uint8_t type;
  uint8_t chs_end[3];
  uint32_t lba_start;
  uint32_t sectors;
};

int getPosition(int value);
int readBlock(int fd, int currentLocation, void *structure, size_t size);

const char *get_partition_type(uint8_t type) {
  switch (type) {
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

void print_chs(uint8_t chs[3]) {
  uint16_t cylinder = ((chs[1] & 0xC0) << 2) | chs[2];
  uint8_t head = chs[0];
  uint8_t sector = chs[1] & 0x3F;
  printf("C: %u, H: %u, S: %u\n", cylinder, head, sector);
}

int main() {
  FILE *file = fopen("imagen.img", "rb");
  if (!file) {
    perror("Error al abrir la imagen");
    return 1;
  }

  struct mbr_partition_entry entry;
  uint32_t sector_size = 512; // Tamaño de sector típico para discos MBR

  // Leer la entrada de la primera partición (0)
  fseek(file, 0x1BE, SEEK_SET);
  fread(&entry, sizeof(entry), 1, file);

  uint64_t partition_size_bytes = entry.sectors * sector_size;
  double partition_size_mb = (double)partition_size_bytes / 1048576;

  printf("Tipo de la partición 0: %s\n", get_partition_type(entry.type));
  printf("Tamaño de la partición 0: %.2f MB\n", partition_size_mb);
  printf("CHS de inicio de la partición 0: ");
  print_chs(entry.chs_start);
  printf("CHS de fin de la partición 0: ");
  print_chs(entry.chs_end);
  printf("LBA de inicio de la partición 0: %u\n", entry.lba_start);

  // Nuevas variables para la segunda parte de la lectura
  struct mbr_partition_entry entry1; // Nueva variable para la segunda partición
  uint64_t partition_size_bytes1; // Nueva variable para el tamaño de la segunda
                                  // partición
  double partition_size_mb1; // Nueva variable para el tamaño en MB de la
                             // segunda partición

  // Leer la entrada de la segunda partición (1)
  fseek(file, 0x1BE + sizeof(struct mbr_partition_entry),
        SEEK_SET); // Posiciona en la entrada de la segunda partición
  fread(&entry1, sizeof(entry1), 1, file); // Lee la entrada de la partición 1

  partition_size_bytes1 = entry1.sectors * sector_size;
  partition_size_mb1 =
      (double)partition_size_bytes1 / 1048576; // Convertir a MB

  printf("\nTipo de la partición 1: %s\n", get_partition_type(entry1.type));
  printf("Tamaño de la partición 1: %.2f MB\n", partition_size_mb1);
  printf("CHS de inicio de la partición 1: ");
  print_chs(entry1.chs_start);
  printf("CHS de fin de la partición 1: ");
  print_chs(entry1.chs_end);
  printf("LBA de inicio de la partición 1: %u\n", entry1.lba_start);

  // Verificar si la partición es de tipo ext4
  if (entry.type != 0x83) {
    printf("La partición 0 no es de tipo ext4\n");
    fclose(file);
    return 1;
  }

  // Obtener superbloque de la partición 0
  uint32_t lba_start0 = entry.lba_start;
  int fd0 = fileno(file);
  struct ext4_super_block sb0;

  // Calcular el superbloque de la partición 0
  int currentLocation = (lba_start0 * sector_size) + 1024;

  // Buscar el superbloque en la partición 0
  if (readBlock(fd0, currentLocation, &sb0, sizeof(sb0)) != 0) {
    fclose(file);
    return 1;
  }

  /*Comienza super bloque de la particion 0*/
  printf("\nComienza el super bloque de la particion 0: \n");
  printf("Número de inodos en la partición 0: %u\n", sb0.s_inodes_count);
  printf("Número de bloques en la partición 0: %u\n", sb0.s_blocks_count_lo);
  printf("Tamaño de bloque en la partición 0: %u\n",
         1024 << sb0.s_log_block_size);
  printf("Firma mágica en la partición 0: %x\n", sb0.s_magic);
  printf("Último punto de montaje en la partición 0: %.64s\n",
         sb0.s_last_mounted);

  //
  printf("Inodes per group: %u\n", sb0.s_inodes_per_group);
  printf("Blocks per group: %u\n", sb0.s_blocks_per_group);

  // TODO: dinamic block size
  // Bloque de descriptores
  currentLocation = currentLocation + 1024; // 1024 bytes del superbloque

  /*
   *  cada descriptor de grupo mide 0x40
   */

  // Leer el bloque de descriptores de grupo
  struct ext4_group_desc groupDescriptor0;
  if (readBlock(fd0, currentLocation, &groupDescriptor0,
                sizeof(groupDescriptor0)) != 0) {
    fclose(file);
    return 1;
  }

  // Impresión de los datos del bloque de descriptores de grupo
  printf("\n DESCRIPTOR DE GRUPOS \n");
  printf("Directorios usados: %u\n", groupDescriptor0.bg_used_dirs_count_lo);
  printf("Bitmap block: %u\n", groupDescriptor0.bg_block_bitmap_lo);
  printf("Inode table: %u\n", groupDescriptor0.bg_inode_table_lo);

  // Posición del primer inode
  int firstInode = getPosition(
      groupDescriptor0
          .bg_inode_table_lo); // obtener la posición del primer inode
  // imprime en hexadecimal
  printf("\nPrimer inode: %x\n", firstInode);

  // Segundo inode (el root)
  int secondInode = firstInode + 0x100; // por cada inode se ocupan 0x100
  printf("Segundo inode: %x\n", secondInode);

  /*
   *  bloque de inode
   */

  // Buscar el bloque de inode
  currentLocation = secondInode;

  // Leer el bloque del inode del root
  struct ext4_inode inode0;
  if (readBlock(fd0, currentLocation, &inode0, sizeof(inode0)) != 0) {
    fclose(file);
    return 1;
  }

  // Impresión de los datos del bloque de extensiones
  printf("\n BLOQUE DE INODE DEL ROOT \n");
  printf("Información del inode para el bloque root: %x\n", inode0.i_block[5]);

  int dirBlock = getPosition(inode0.i_block[5]);
  printf("El bloque del root esta en: %x\n", dirBlock);

  /*
   *  bloque del directorio root
   */
  // Posicionar la ubicación actual en el bloque del directorio root
  currentLocation = dirBlock;

  struct ext4_dir_entry_2 root;

  // Leer el bloque del directorio root
  if (readBlock(fd0, currentLocation, &root, sizeof(root)) != 0) {
    fclose(file);
    return 1;
  }

  // TODO: fix this
  struct ext4_dir_entry_2 segundoDirectorioAux;

  // Impresión de los datos del bloque root
  printf("\n BLOQUE DEL DIRECTORIO ROOT \n");

  char *blockPtr = (char *)&root;
  while (blockPtr < (char *)&root + sizeof(root)) {
    struct ext4_dir_entry_2 *entry = (struct ext4_dir_entry_2 *)blockPtr;

    // TODO: remove this
    if(entry->inode == 32642) {
      segundoDirectorioAux = *entry;
    }

    printf("Nombre: %s -> %u\n", entry->name, entry->inode);

    // Move to the next directory entry
    blockPtr += entry->rec_len;
  }

  /*
  * Inodes de los directorios volviendo a los descriptores de grupos
  */
 printf("Segundo directorio inode: %u\n", segundoDirectorioAux.inode);

 // TODO: division con residuo
 // Calcular el descriptor de grupo para el segundo directorio
 int calculoDescriptorDeGrupo = segundoDirectorioAux.inode / 2040; // 2040 = 0x7F8, tamaño de un grupo
 // Se multiplica por 0x40 porque cada descriptor de grupo mide 0x40
 calculoDescriptorDeGrupo = calculoDescriptorDeGrupo * 0x40;
 // Se suma 0x100800 porque es la posición donde comienzan los descriptores de grupo
 calculoDescriptorDeGrupo = calculoDescriptorDeGrupo + 0x100800;
 printf("Calculo desconocido: %x\n", calculoDescriptorDeGrupo);

 currentLocation = calculoDescriptorDeGrupo;

  // TODO: reusar la variable anterior
  // Leer el bloque de descriptores de grupo
  struct ext4_group_desc groupDescriptor1;

  if (readBlock(fd0, currentLocation, &groupDescriptor1, sizeof(groupDescriptor1)) != 0) {
    fclose(file);
    return 1;
  }

  // TODO: remove this
  // Impresión de los datos del bloque de descriptores de grupo
  printf("\n DESCRIPTOR DE GRUPOS \n");
  printf("Directorios usados: %x\n", groupDescriptor1.bg_used_dirs_count_lo);
  printf("Bitmap block: %x\n", groupDescriptor1.bg_block_bitmap_lo);
  printf("Inode table: %x\n", groupDescriptor1.bg_inode_table_lo);

  // Obtener la tabla de inodes
  currentLocation = getPosition(groupDescriptor1.bg_inode_table_lo);
  printf("Tabla de inodes: %x\n", currentLocation);

  // Leer el bloque de inode del segundo directorio
  struct ext4_inode inode1;
  if (readBlock(fd0, currentLocation, &inode1, sizeof(inode1)) != 0) {
    fclose(file);
    return 1;
  }

  printf("\n BLOQUE DE INODE DEL ROOT \n");
  printf("Información del inode para el bloque: %x\n", inode1.i_block[5]); // TODO: Porqué es 5?

  dirBlock = getPosition(inode1.i_block[5]);
  printf("El bloque del directorio esta en: %x\n", dirBlock);

  
  /*
   *  bloque del segundo directorio
   */
  // Posicionar la ubicación actual en el bloque del segundo directorio
  currentLocation = dirBlock;

  struct ext4_dir_entry_2 directorio;

  // Leer el bloque del segundo directorio
  if (readBlock(fd0, currentLocation, &directorio, sizeof(directorio)) != 0) {
    fclose(file);
    return 1;
  }

  // Impresión de los datos del bloque root
  printf("\n BLOQUE DEL SEGUNDO DIRECTORIO\n");

  // TODO: what's does mean 'blockPtr'?
  char *blockPtrD = (char *)&directorio;
  while (blockPtrD < (char *)&directorio + sizeof(directorio)) {
    struct ext4_dir_entry_2 *entry = (struct ext4_dir_entry_2 *)blockPtrD;

    printf("Nombre: %s -> %u\n", entry->name, entry->inode);

    // Move to the next directory entry
    blockPtrD += entry->rec_len;
  }


  fclose(file);
  return 0;
}

int getPosition(int value) { return value * 0x400 + 0x100000; }

int readBlock(int fd, int currentLocation, void *structure, size_t size) {
  if (lseek(fd, currentLocation, SEEK_SET) < 0) {
    perror("Error al buscar el bloque");
    return 1;
  }

  if (read(fd, structure, size) != size) {
    perror("Error al leer el bloque");
    return 1;
  }

  return 0;
}