#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <windows.h>
#include <time.h>

#pragma pack(push, 1)
typedef struct {
  uint8_t  BS_jmpBoot[3];
  uint8_t  BS_OEMName[8];
  uint16_t BPB_BytsPerSec;
  uint8_t  BPB_SecPerClus;
  uint16_t BPB_RsvdSecCnt;
  uint8_t  BPB_NumFATs;
  uint16_t BPB_RootEntCnt;
  uint16_t BPB_TotSec16;
  uint8_t  BPB_Media;
  uint16_t BPB_FATSz16;
  uint16_t BPB_SecPerTrk;
  uint16_t BPB_NumHeads;
  uint32_t BPB_HiddSec;
  uint32_t BPB_TotSec32;
  uint8_t  BS_DrvNum;
  uint8_t  BS_Reserved1;
  uint8_t  BS_BootSig;
  uint32_t BS_VolID;
  uint8_t  BS_VolLab[11];
  uint8_t  BS_FilSysType[8];
} FAT16BootSector;

typedef struct {
  uint8_t  DIR_Name[11];
  uint8_t  DIR_Attr;
  uint8_t  DIR_NTRes;
  uint8_t  DIR_CrtTimeTenth;
  uint16_t DIR_CrtTime;
  uint16_t DIR_CrtDate;
  uint16_t DIR_LstAccDate;
  uint16_t DIR_FstClusHI;
  uint16_t DIR_WrtTime;
  uint16_t DIR_WrtDate;
  uint16_t DIR_FstClusLO;
  uint32_t DIR_FileSize;
} FAT16DirectoryEntry;
#pragma pack(pop)

void format_file_name(const char *input, char *output) {
  int i = 0, j = 0;

  // Copiar nome (até 8 caracteres)
  while (i < 8 && input[j] != '\0' && input[j] != '.') {
    output[i] = toupper(input[j]);
    i++;
    j++;
  }

  // Preencher o restante com espaços
  while (i < 8) {
    output[i] = ' ';
    i++;
  }

  // Se houver um ponto, avançar para a extensão
  if (input[j] == '.') {
    j++;
  }

  // Copiar extensão (até 3 caracteres)
  while (i < 11 && input[j] != '\0') {
    output[i] = toupper(input[j]);
    i++;
    j++;
  }

  // Preencher o restante com espaços
  while (i < 11) {
    output[i] = ' ';
    i++;
  }

  // Adicionar caractere nulo no final
  output[11] = '\0';
}

void list_root_directory(FILE *disk, FAT16BootSector *bootSector) {
  int rootDirSectors = ((bootSector->BPB_RootEntCnt * 32) + (bootSector->BPB_BytsPerSec - 1)) / bootSector->BPB_BytsPerSec;
  int firstRootDirSecNum = bootSector->BPB_RsvdSecCnt + (bootSector->BPB_NumFATs * bootSector->BPB_FATSz16);
  int rootDirSize = bootSector->BPB_RootEntCnt * sizeof(FAT16DirectoryEntry);

  FAT16DirectoryEntry *rootDirectory = (FAT16DirectoryEntry *)malloc(rootDirSize);
  if (!rootDirectory) {
    printf("Erro ao alocar memória para o diretório raiz.\n");
    return;
  }

  fseek(disk, firstRootDirSecNum * bootSector->BPB_BytsPerSec, SEEK_SET);
  fread(rootDirectory, rootDirSize, 1, disk);

  printf("Conteúdo do diretório raiz:\n");
  for (int i = 0; i < bootSector->BPB_RootEntCnt; i++) {
    if (rootDirectory[i].DIR_Name[0] != 0x00 && rootDirectory[i].DIR_Name[0] != 0xE5) {
      char fileName[12];
      memset(fileName, 0, sizeof(fileName));
      strncpy(fileName, (char *)rootDirectory[i].DIR_Name, 11);

      // Remover espaços em branco do nome do arquivo
      for (int j = 10; j >= 0; j--) {
        if (fileName[j] == ' ') {
          fileName[j] = '\0';
        } else {
          break;
        }
      }

      printf("Nome do Arquivo: %s, Tamanho do Arquivo: %u bytes\n", fileName, rootDirectory[i].DIR_FileSize);
    }
  }

  free(rootDirectory);
}

void show_file_attributes(FILE *disk, FAT16BootSector *bootSector, const char *fileName) {
  int rootDirSectors = ((bootSector->BPB_RootEntCnt * 32) + (bootSector->BPB_BytsPerSec - 1)) / bootSector->BPB_BytsPerSec;
  int firstRootDirSecNum = bootSector->BPB_RsvdSecCnt + (bootSector->BPB_NumFATs * bootSector->BPB_FATSz16);
  int rootDirSize = bootSector->BPB_RootEntCnt * sizeof(FAT16DirectoryEntry);

  FAT16DirectoryEntry *rootDirectory = (FAT16DirectoryEntry *)malloc(rootDirSize);
  if (!rootDirectory) {
    printf("Erro ao alocar memória para o diretório raiz.\n");
    return;
  }

  fseek(disk, firstRootDirSecNum * bootSector->BPB_BytsPerSec, SEEK_SET);
  fread(rootDirectory, rootDirSize, 1, disk);

  // Formatar o nome do arquivo para o formato 8.3 em maiúsculas
  char formattedFileName[12];
  format_file_name(fileName, formattedFileName);

  FAT16DirectoryEntry *fileEntry = NULL;
  for (int i = 0; i < bootSector->BPB_RootEntCnt; i++) {
    if (rootDirectory[i].DIR_Name[0] != 0x00 && rootDirectory[i].DIR_Name[0] != 0xE5) {
      if (strncmp((char *)rootDirectory[i].DIR_Name, formattedFileName, 11) == 0) {
        fileEntry = &rootDirectory[i];
        break;
      }
    }
  }

  if (fileEntry) {
    printf("Atributos do arquivo %s:\n", fileName);

    // Atributos
    printf("Atributos: ");
    if (fileEntry->DIR_Attr & 0x01) { printf("Somente Leitura "); }else{ printf(" Editavel, ");}
    if (fileEntry->DIR_Attr & 0x02) { printf("Oculto "); }else{ printf("Visivel, ");}
    if (fileEntry->DIR_Attr & 0x04) { printf("Sistema "); }else{ printf("Não sistema. ");}
    printf("\n");

    // Data/Hora de Criação
    uint16_t createDate = fileEntry->DIR_CrtDate;
    uint16_t createTime = fileEntry->DIR_CrtTime;
    printf("Data/Hora de Criação: %02d/%02d/%d %02d:%02d:%02d\n",
           createDate & 0x1F, (createDate >> 5) & 0x0F, (createDate >> 9) + 1980,
           (createTime >> 11) & 0x1F, (createTime >> 5) & 0x3F, (createTime & 0x1F) * 2);

    // Data/Hora de Modificação
    uint16_t writeDate = fileEntry->DIR_WrtDate;
    uint16_t writeTime = fileEntry->DIR_WrtTime;
    printf("Data/Hora de Modificação: %02d/%02d/%d %02d:%02d:%02d\n",
           writeDate & 0x1F, (writeDate >> 5) & 0x0F, (writeDate >> 9) + 1980,
           (writeTime >> 11) & 0x1F, (writeTime >> 5) & 0x3F, (writeTime & 0x1F) * 2);
  } else {
    printf("Arquivo %s não encontrado.\n", fileName);
  }

  free(rootDirectory);
}

void show_file_content(FILE *disk, FAT16BootSector *bootSector, const char *fileName) {
  int rootDirSectors = ((bootSector->BPB_RootEntCnt * 32) + (bootSector->BPB_BytsPerSec - 1)) / bootSector->BPB_BytsPerSec;
  int firstRootDirSecNum = bootSector->BPB_RsvdSecCnt + (bootSector->BPB_NumFATs * bootSector->BPB_FATSz16);
  int rootDirSize = bootSector->BPB_RootEntCnt * sizeof(FAT16DirectoryEntry);

  FAT16DirectoryEntry *rootDirectory = (FAT16DirectoryEntry *)malloc(rootDirSize);
  if (!rootDirectory) {
    printf("Erro ao alocar memória para o diretório raiz.\n");
    return;
  }

  fseek(disk, firstRootDirSecNum * bootSector->BPB_BytsPerSec, SEEK_SET);
  fread(rootDirectory, rootDirSize, 1, disk);

  // Formatar o nome do arquivo para o formato 8.3 em maiúsculas
  char formattedFileName[12];
  format_file_name(fileName, formattedFileName);

  FAT16DirectoryEntry *fileEntry = NULL;
  for (int i = 0; i < bootSector->BPB_RootEntCnt; i++) {
    if (rootDirectory[i].DIR_Name[0] != 0x00 && rootDirectory[i].DIR_Name[0] != 0xE5) {
      if (strncmp((char *)rootDirectory[i].DIR_Name, formattedFileName, 11) == 0) {
        fileEntry = &rootDirectory[i];
        break;
      }
    }
  }

  if (fileEntry) {
    printf("Conteúdo do arquivo %s:\n", fileName);

    uint16_t cluster = fileEntry->DIR_FstClusLO;
    uint32_t fileSize = fileEntry->DIR_FileSize;
    uint8_t *buffer = (uint8_t *)malloc(bootSector->BPB_BytsPerSec * bootSector->BPB_SecPerClus);

    if (!buffer) {
      printf("Erro ao alocar memória para o buffer de leitura do arquivo.\n");
      free(rootDirectory);
      return;
    }

    while (fileSize > 0) {
      int firstDataSector = bootSector->BPB_RsvdSecCnt + (bootSector->BPB_NumFATs * bootSector->BPB_FATSz16) + rootDirSectors;
      int firstSectorOfCluster = firstDataSector + (cluster - 2) * bootSector->BPB_SecPerClus;

      fseek(disk, firstSectorOfCluster * bootSector->BPB_BytsPerSec, SEEK_SET);
      fread(buffer, bootSector->BPB_BytsPerSec, bootSector->BPB_SecPerClus, disk);

      uint32_t bytesToPrint = (fileSize < bootSector->BPB_BytsPerSec * bootSector->BPB_SecPerClus) ? fileSize : bootSector->BPB_BytsPerSec * bootSector->BPB_SecPerClus;
      fwrite(buffer, 1, bytesToPrint, stdout);

      fileSize -= bytesToPrint;

      // Simular leitura da FAT para o próximo cluster (FAT16, cluster size fixo de 2 bytes)
      // Esta parte do código deve ser ajustada para lidar corretamente com a FAT
      // Aqui, estou simulando a leitura de um cluster contínuo, que pode não ser o caso em uma FAT real
      cluster++; // Incrementar para o próximo cluster (simplificação)
    }

    free(buffer);
  } else {
    printf("Arquivo %s não encontrado.\n", fileName);
  }

  free(rootDirectory);
}

void create_new_file(FILE *disk, FAT16BootSector *bootSector, const char *fileName, const char *externalFileName) {
  int rootDirSectors = ((bootSector->BPB_RootEntCnt * 32) + (bootSector->BPB_BytsPerSec - 1)) / bootSector->BPB_BytsPerSec;
  int firstRootDirSecNum = bootSector->BPB_RsvdSecCnt + (bootSector->BPB_NumFATs * bootSector->BPB_FATSz16);
  int rootDirSize = bootSector->BPB_RootEntCnt * sizeof(FAT16DirectoryEntry);

  // Ler o conteúdo do arquivo externo
  FILE *externalFile = fopen(externalFileName, "rb");
  if (!externalFile) {
    printf("Erro ao abrir o arquivo externo.\n");
    return;
  }
  fseek(externalFile, 0, SEEK_END);
  long externalFileSize = ftell(externalFile);
  fseek(externalFile, 0, SEEK_SET);

  uint8_t *externalFileContent = (uint8_t *)malloc(externalFileSize);
  if (!externalFileContent) {
    printf("Erro ao alocar memória para o conteúdo do arquivo externo.\n");
    fclose(externalFile);
    return;
  }
  fread(externalFileContent, 1, externalFileSize, externalFile);
  fclose(externalFile);

  // Ler o diretório raiz
  FAT16DirectoryEntry *rootDirectory = (FAT16DirectoryEntry *)malloc(rootDirSize);
  if (!rootDirectory) {
    printf("Erro ao alocar memória para o diretório raiz.\n");
    free(externalFileContent);
    return;
  }
  fseek(disk, firstRootDirSecNum * bootSector->BPB_BytsPerSec, SEEK_SET);
  fread(rootDirectory, rootDirSize, 1, disk);

  // Encontrar uma entrada vazia no diretório raiz
  FAT16DirectoryEntry *emptyEntry = NULL;
  for (int i = 0; i < bootSector->BPB_RootEntCnt; i++) {
    if (rootDirectory[i].DIR_Name[0] == 0x00 || rootDirectory[i].DIR_Name[0] == 0xE5) {
      emptyEntry = &rootDirectory[i];
      break;
    }
  }
  if (!emptyEntry) {
    printf("Não há espaço suficiente no diretório raiz.\n");
    free(externalFileContent);
    free(rootDirectory);
    return;
  }

  // Formatar o nome do arquivo para o formato 8.3 em maiúsculas
  char formattedFileName[12];
  format_file_name(fileName, formattedFileName);
  strncpy((char *)emptyEntry->DIR_Name, formattedFileName, 11);

  // Encontrar clusters livres na FAT
  int fatSize = bootSector->BPB_FATSz16 * bootSector->BPB_BytsPerSec;
  uint16_t *fat = (uint16_t *)malloc(fatSize);
  if (!fat) {
    printf("Erro ao alocar memória para a FAT.\n");
    free(externalFileContent);
    free(rootDirectory);
    return;
  }
  fseek(disk, bootSector->BPB_RsvdSecCnt * bootSector->BPB_BytsPerSec, SEEK_SET);
  fread(fat, fatSize, 1, disk);

  int clusterCount = (externalFileSize + bootSector->BPB_BytsPerSec * bootSector->BPB_SecPerClus - 1) / (bootSector->BPB_BytsPerSec * bootSector->BPB_SecPerClus);
  uint16_t *clusters = (uint16_t *)malloc(clusterCount * sizeof(uint16_t));
  if (!clusters) {
    printf("Erro ao alocar memória para os clusters.\n");
    free(fat);
    free(externalFileContent);
    free(rootDirectory);
    return;
  }

  int clusterIndex = 0;
  for (int i = 2; i < fatSize / 2; i++) {
    if (fat[i] == 0x0000) {
      clusters[clusterIndex++] = i;
      if (clusterIndex == clusterCount) {
        break;
      }
    }
  }
  if (clusterIndex < clusterCount) {
    printf("Não há clusters livres suficientes na FAT.\n");
    free(clusters);
    free(fat);
    free(externalFileContent);
    free(rootDirectory);
    return;
  }

  // Escrever o conteúdo do arquivo nos clusters livres
  int firstDataSector = bootSector->BPB_RsvdSecCnt + (bootSector->BPB_NumFATs * bootSector->BPB_FATSz16) + rootDirSectors;
  for (int i = 0; i < clusterCount; i++) {
    int firstSectorOfCluster = firstDataSector + (clusters[i] - 2) * bootSector->BPB_SecPerClus;
    fseek(disk, firstSectorOfCluster * bootSector->BPB_BytsPerSec, SEEK_SET);
    fwrite(externalFileContent + i * bootSector->BPB_BytsPerSec * bootSector->BPB_SecPerClus, 1, bootSector->BPB_BytsPerSec * bootSector->BPB_SecPerClus, disk);
  }

  // Atualizar a FAT
  for (int i = 0; i < clusterCount - 1; i++) {
    fat[clusters[i]] = clusters[i + 1];
  }
  fat[clusters[clusterCount - 1]] = 0xFFFF; // End of file
  fseek(disk, bootSector->BPB_RsvdSecCnt * bootSector->BPB_BytsPerSec, SEEK_SET);
  fwrite(fat, fatSize, 1, disk);

  // Atualizar a entrada do diretório raiz
  emptyEntry->DIR_Attr = 0x20; // Arquivo
  emptyEntry->DIR_FileSize = externalFileSize;
  emptyEntry->DIR_FstClusLO = clusters[0];

  // Obter a data e hora atuais
  SYSTEMTIME st;
  GetSystemTime(&st);
  uint16_t date = ((st.wYear - 1980) << 9) | (st.wMonth << 5) | st.wDay;
  uint16_t time = (st.wHour << 11) | (st.wMinute << 5) | (st.wSecond / 2);

  emptyEntry->DIR_CrtDate = date;
  emptyEntry->DIR_CrtTime = time;
  emptyEntry->DIR_WrtDate = date;
  emptyEntry->DIR_WrtTime = time;

  fseek(disk, firstRootDirSecNum * bootSector->BPB_BytsPerSec, SEEK_SET);
  fwrite(rootDirectory, rootDirSize, 1, disk);

  // Limpar memória
  free(clusters);
  free(fat);
  free(externalFileContent);
  free(rootDirectory);

  printf("Arquivo %s criado com sucesso.\n", fileName);
}

void delete_file(FILE *disk, FAT16BootSector *bootSector, const char *fileName) {
  int rootDirSectors = ((bootSector->BPB_RootEntCnt * 32) + (bootSector->BPB_BytsPerSec - 1)) / bootSector->BPB_BytsPerSec;
  int firstRootDirSecNum = bootSector->BPB_RsvdSecCnt + (bootSector->BPB_NumFATs * bootSector->BPB_FATSz16);
  int rootDirSize = bootSector->BPB_RootEntCnt * sizeof(FAT16DirectoryEntry);

  // Ler o diretório raiz
  FAT16DirectoryEntry *rootDirectory = (FAT16DirectoryEntry *)malloc(rootDirSize);
  if (!rootDirectory) {
    printf("Erro ao alocar memória para o diretório raiz.\n");
    return;
  }
  fseek(disk, firstRootDirSecNum * bootSector->BPB_BytsPerSec, SEEK_SET);
  fread(rootDirectory, rootDirSize, 1, disk);

  // Formatar o nome do arquivo para o formato 8.3 em maiúsculas
  char formattedFileName[12];
  format_file_name(fileName, formattedFileName);

  FAT16DirectoryEntry *fileEntry = NULL;
  for (int i = 0; i < bootSector->BPB_RootEntCnt; i++) {
    if (rootDirectory[i].DIR_Name[0] != 0x00 && rootDirectory[i].DIR_Name[0] != 0xE5) {
      if (strncmp((char *)rootDirectory[i].DIR_Name, formattedFileName, 11) == 0) {
        fileEntry = &rootDirectory[i];
        break;
      }
    }
  }

  if (fileEntry) {
    // Marcar a entrada do diretório como excluída
    fileEntry->DIR_Name[0] = 0xE5;

    // Atualizar a FAT para liberar clusters
    int fatSize = bootSector->BPB_FATSz16 * bootSector->BPB_BytsPerSec;
    uint16_t *fat = (uint16_t *)malloc(fatSize);
    if (!fat) {
      printf("Erro ao alocar memória para a FAT.\n");
      free(rootDirectory);
      return;
    }
    fseek(disk, bootSector->BPB_RsvdSecCnt * bootSector->BPB_BytsPerSec, SEEK_SET);
    fread(fat, fatSize, 1, disk);

    uint16_t cluster = fileEntry->DIR_FstClusLO;
    while (cluster < 0xFFF8) {
      uint16_t nextCluster = fat[cluster];
      fat[cluster] = 0x0000; // Marcar como livre
      cluster = nextCluster;
    }

    fseek(disk, bootSector->BPB_RsvdSecCnt * bootSector->BPB_BytsPerSec, SEEK_SET);
    fwrite(fat, fatSize, 1, disk);
    free(fat);

    // Atualizar o diretório raiz
    fseek(disk, firstRootDirSecNum * bootSector->BPB_BytsPerSec, SEEK_SET);
    fwrite(rootDirectory, rootDirSize, 1, disk);

    printf("Arquivo %s removido com sucesso.\n", fileName);
  } else {
    printf("Arquivo %s não encontrado.\n", fileName);
  }

  free(rootDirectory);
}

void rename_file(FILE *disk, FAT16BootSector *bootSector, const char *oldFileName, const char *newFileName) {
  int rootDirSectors = ((bootSector->BPB_RootEntCnt * 32) + (bootSector->BPB_BytsPerSec - 1)) / bootSector->BPB_BytsPerSec;
  int firstRootDirSecNum = bootSector->BPB_RsvdSecCnt + (bootSector->BPB_NumFATs * bootSector->BPB_FATSz16);
  int rootDirSize = bootSector->BPB_RootEntCnt * sizeof(FAT16DirectoryEntry);

  // Ler o diretório raiz
  FAT16DirectoryEntry *rootDirectory = (FAT16DirectoryEntry *)malloc(rootDirSize);
  if (!rootDirectory) {
    printf("Erro ao alocar memória para o diretório raiz.\n");
    return;
  }
  fseek(disk, firstRootDirSecNum * bootSector->BPB_BytsPerSec, SEEK_SET);
  fread(rootDirectory, rootDirSize, 1, disk);

  // Formatar o nome dos arquivos para o formato 8.3 em maiúsculas
  char formattedOldFileName[12];
  char formattedNewFileName[12];
  format_file_name(oldFileName, formattedOldFileName);
  format_file_name(newFileName, formattedNewFileName);

  FAT16DirectoryEntry *fileEntry = NULL;
  for (int i = 0; i < bootSector->BPB_RootEntCnt; i++) {
    if (rootDirectory[i].DIR_Name[0] != 0x00 && rootDirectory[i].DIR_Name[0] != 0xE5) {
      if (strncmp((char *)rootDirectory[i].DIR_Name, formattedOldFileName, 11) == 0) {
        fileEntry = &rootDirectory[i];
        break;
      }
    }
  }

  if (fileEntry) {
    // Atualizar o nome do arquivo
    strncpy((char *)fileEntry->DIR_Name, formattedNewFileName, 11);

    // Atualizar a data e hora de modificação
    SYSTEMTIME st;
    GetSystemTime(&st);
    uint16_t date = ((st.wYear - 1980) << 9) | (st.wMonth << 5) | st.wDay;
    uint16_t time = (st.wHour << 11) | (st.wMinute << 5) | (st.wSecond / 2);

    fileEntry->DIR_WrtDate = date;
    fileEntry->DIR_WrtTime = time;

    // Atualizar o diretório raiz
    fseek(disk, firstRootDirSecNum * bootSector->BPB_BytsPerSec, SEEK_SET);
    fwrite(rootDirectory, rootDirSize, 1, disk);

    printf("Arquivo %s renomeado para %s com sucesso.\n", oldFileName, newFileName);
  } else {
    printf("Arquivo %s não encontrado.\n", oldFileName);
  }

  free(rootDirectory);
}

void show_menu() {
  printf("\nMenu:\n");
  printf("1. Listar conteúdo do disco\n");
  printf("2. Mostrar conteúdo de um arquivo\n");
  printf("3. Exibir atributos de um arquivo\n");
  printf("4. Inserir/criar um novo arquivo\n");
  printf("5. Apagar/remover um arquivo\n");
  printf("6. Renomear um arquivo\n");
  printf("7. Sair\n");
  printf("Escolha uma opção: ");
}

int main() {
  // Define a página de código para UTF-8
  SetConsoleOutputCP(CP_UTF8);

  // Abre o arquivo de imagem do disco
  FILE *disk = fopen("C:\\Users\\berna\\CLionProjects\\T2_SO_Bernardo_LeoBorges_Igor\\disco1.img", "r+b");
  if (!disk) {
    printf("Erro ao abrir a imagem do disco.\n");
    return 1;
  }

  FAT16BootSector bootSector;
  fread(&bootSector, sizeof(FAT16BootSector), 1, disk);

  int option;
  do {
    show_menu();
    scanf("%d", &option);

    switch (option) {
      case 1:
        list_root_directory(disk, &bootSector);
        break;
      case 2: {
        char fileName[12];
        printf("Digite o nome do arquivo: ");
        scanf("%s", fileName); // Leitura do nome do arquivo
        show_file_content(disk, &bootSector, fileName);
        break;
      }
      case 3: {
        char fileName[12];
        printf("Digite o nome do arquivo: ");
        scanf("%s", fileName); // Leitura do nome do arquivo
        show_file_attributes(disk, &bootSector, fileName);
        break;
      }
      case 4: {
        char fileName[12];
        char externalFileName[100];
        printf("Digite o nome do novo arquivo: ");
        scanf("%s", fileName); // Leitura do nome do novo arquivo
        printf("Digite o caminho do arquivo externo: ");
        scanf("%s", externalFileName); // Leitura do caminho do arquivo externo
        create_new_file(disk, &bootSector, fileName, externalFileName);
        break;
      }
      case 5: {
        char fileName[12];
        printf("Digite o nome do arquivo a ser removido: ");
        scanf("%s", fileName); // Leitura do nome do arquivo
        delete_file(disk, &bootSector, fileName);
        break;
      }
      case 6: {
        char oldFileName[12];
        char newFileName[12];
        printf("Digite o nome atual do arquivo: ");
        scanf("%s", oldFileName); // Leitura do nome atual do arquivo
        printf("Digite o novo nome do arquivo: ");
        scanf("%s", newFileName); // Leitura do novo nome do arquivo
        rename_file(disk, &bootSector, oldFileName, newFileName);
        break;
      }
      case 7:
        printf("Saindo...\n");
        break;
      default:
        printf("Opção inválida. Tente novamente.\n");
    }
  } while (option != 7);

  fclose(disk);
  return 0;
}
