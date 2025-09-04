#ifndef BANNER_H
#define BANNER_H

void print_banner() {
    printf("\033[1;32m"); // Set text color to green
    printf("  _____ _           _   _       _     _           \n");
    printf(" / ____| |         | | | |     | |   | |          \n");
    printf("| |    | |__   __ _| |_| |__   | |__ | | ___  ___ \n");
    printf("| |    | '_ \\ / _` | __| '_ \\  | '_ \\| |/ _ \\/ __|\n");
    printf("| |____| | | | (_| | |_| | | | | |_) | |  __/\\__ \\\n");
    printf(" \\_____|_| |_|\\__,_|\\__|_| |_| |_.__/|_|\\___||___/\n");
    printf("                                                  \n");
    printf("        Global Multi-Client Chat System           \n");
    printf("\033[0m"); // Reset text color
}

#endif
