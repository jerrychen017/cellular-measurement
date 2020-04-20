#include "data_generator.h"

/**
    Data generator
*/
int main(int argc, char *argv[])
{
    // args error checking
    if (argc != 1)
    {
        printf("data_generator usage: data_generator\n");
        exit(1);
    }

    int ret = start_generator(false); 
    return ret; 
}