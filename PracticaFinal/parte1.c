#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/fs.h>
#include "ext4.h"

struct mbr_partition_entry
{
    uint8_t status;
    uint8_t chs_start[3];
    uint8_t type;
    uint8_t chs_end[3];
    uint32_t lba_start;
    uint32_t sectors;
};

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

void print_chs(uint8_t chs[3])
{
    uint16_t cylinder = ((chs[1] & 0xC0) << 2) | chs[2];
    uint8_t head = chs[0];
    uint8_t sector = chs[1] & 0x3F;
    printf("C: %u, H: %u, S: %u\n", cylinder, head, sector);
}

int main()
{
    FILE *file = fopen("imagen.img", "rb");
    if (!file)
    {
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
    uint64_t partition_size_bytes1;    // Nueva variable para el tamaño de la segunda partición
    double partition_size_mb1;         // Nueva variable para el tamaño en MB de la segunda partición

    // Leer la entrada de la segunda partición (1)
    fseek(file, 0x1BE + sizeof(struct mbr_partition_entry), SEEK_SET); // Posiciona en la entrada de la segunda partición
    fread(&entry1, sizeof(entry1), 1, file);                           // Lee la entrada de la partición 1

    partition_size_bytes1 = entry1.sectors * sector_size;
    partition_size_mb1 = (double)partition_size_bytes1 / 1048576; // Convertir a MB

    printf("\nTipo de la partición 1: %s\n", get_partition_type(entry1.type));
    printf("Tamaño de la partición 1: %.2f MB\n", partition_size_mb1);
    printf("CHS de inicio de la partición 1: ");
    print_chs(entry1.chs_start);
    printf("CHS de fin de la partición 1: ");
    print_chs(entry1.chs_end);
    printf("LBA de inicio de la partición 1: %u\n", entry1.lba_start);

    // Verificar si la partición es de tipo ext4
    if (entry.type != 0x83)
    {
        printf("La partición 0 no es de tipo ext4\n");
        fclose(file);
        return 1;
    }

    // Obtener superbloque de la partición 0
    uint32_t lba_start0 = entry.lba_start;
    int fd0 = fileno(file);
    struct ext4_super_block sb0;

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
    /*Comienza super bloque de la particion 0*/
    printf("\nComienza el super bloque de la particion 0: \n");
    printf("Número de inodos en la partición 0: %u\n", sb0.s_inodes_count);
    printf("Número de bloques en la partición 0: %u\n", sb0.s_blocks_count_lo);
    printf("Tamaño de bloque en la partición 0: %u\n", 1024 << sb0.s_log_block_size);
    printf("Firma mágica en la partición 0: %x\n", sb0.s_magic);
    printf("Último punto de montaje en la partición 0: %.64s\n", sb0.s_last_mounted);

    fclose(file);
    return 0;
}
