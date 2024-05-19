#include <stdio.h>
#include <stdint.h>

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

    // Leer la entrada de la primera partición (0)
    fseek(file, 0x1BE, SEEK_SET); // Posiciona en la tabla de particiones MBR
    fread(&entry, sizeof(entry), 1, file); // Lee la entrada de la partición 0

    uint32_t sector_size = 512; // Tamaño de sector típico para discos MBR
    uint64_t partition_size_bytes = entry.sectors * sector_size;
    double partition_size_mb = (double)partition_size_bytes / 1048576; // Convertir a MB

    printf("Tipo de la partición 0: %s\n", get_partition_type(entry.type));
    printf("Tamaño de la partición 0: %.2f MB\n", partition_size_mb);
    printf("CHS de inicio de la partición 0: ");
    print_chs(entry.chs_start);
    printf("CHS de fin de la partición 0: ");
    print_chs(entry.chs_end);

    // Leer la entrada de la segunda partición (1)
    fseek(file, 0x1BE + sizeof(struct mbr_partition_entry), SEEK_SET); // Posiciona en la entrada de la segunda partición
    fread(&entry, sizeof(entry), 1, file); // Lee la entrada de la partición 1

    partition_size_bytes = entry.sectors * sector_size;
    partition_size_mb = (double)partition_size_bytes / 1048576; // Convertir a MB

    printf("Tipo de la partición 1: %s\n", get_partition_type(entry.type));
    printf("Tamaño de la partición 1: %.2f MB\n", partition_size_mb);
    printf("CHS de inicio de la partición 1: ");
    print_chs(entry.chs_start);
    printf("CHS de fin de la partición 1: ");
    print_chs(entry.chs_end);

    fclose(file);
    return 0;
}
