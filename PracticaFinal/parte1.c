#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/fs.h>
#include "ext4.h"

struct mbr_partition_entry {
    uint8_t status;
    uint8_t chs_start[3];
    uint8_t type;
    uint8_t chs_end[3];
    uint32_t lba_start;
    uint32_t sectors;
};

const char* get_partition_type(uint8_t type) {
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
        default:   return "Unknown";
    }
}

void print_chs(uint8_t chs[3]) {
    uint16_t cylinder = ((chs[1] & 0xC0) << 2) | chs[2];
    uint8_t head = chs[0];
    uint8_t sector = chs[1] & 0x3F;
    printf("C: %u, H: %u, S: %u\n", cylinder, head, sector);
}

int main() {
    FILE* file = fopen("imagen.img", "rb");
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

    // Verificar si la partición es de tipo ext4
    if (entry.type != 0x83) {
        printf("La partición 0 no es de tipo ext4\n");
        fclose(file);
        return 1;
    }

    // Obtener superbloque de la partición 0
    uint32_t lba_start = entry.lba_start;
    int fd = fileno(file);
    struct ext4_super_block sb;

    // Buscar el superbloque en la partición 0
    if (lseek(fd, (lba_start * sector_size) + 1024, SEEK_SET) < 0) {
        perror("Error al buscar el superbloque en la partición 0");
        fclose(file);
        return 1;
    }

    if (read(fd, &sb, sizeof(sb)) != sizeof(sb)) {
        perror("Error al leer el superbloque en la partición 0");
        fclose(file);
        return 1;
    }
    printf("\nComienza superbloque de la particion 0: \n");
    printf("Cuenta inodes 0: %u\n", sb.s_inodes_count);
    printf("Cuenta Blocks 0: %u\n", sb.s_blocks_count_lo);
    printf("Block size de 0: %u\n", 1024 << sb.s_log_block_size);
    printf("Magic Number 0: %x\n", sb.s_magic);

    fclose(file);
    return 0;
}
