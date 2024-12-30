#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CHOICE_LEN 64
#define MAX_FILE_NAME 128
#define MAX_BUFFER_LEN 10000

#define max(a, b) ((a) > (b) ? (a) : (b))

static char *hex_formats[] = {"%#hhx\n", "%#hx\n", "No such unit", "%#x\n"};
static char *dec_formats[] = {"%#hhd\n", "%#hd\n", "No such unit", "%#d\n"};

typedef struct
{
    char debug_mode;
    char file_name[MAX_FILE_NAME];
    int unit_size;
    unsigned char mem_buf[MAX_BUFFER_LEN];
    size_t mem_count;
    char display_mode; // 0: dec, 1: hex
} state;

typedef struct fun_desc
{
    char *name;
    void (*fun)(state *);
} fun_desc;

void reset_mem_buf(state *s)
{
    memset(s->mem_buf, 0, s->mem_count);
    s->mem_count = 0;
}

void toggle_debug_mode(state *s)
{
    s->debug_mode ^= 1;
    fprintf(stderr, "Debug flag now %s\n", s->debug_mode ? "on" : "off");
}

void set_file_name(state *s)
{
    printf("Enter file name:\n");
    char input[MAX_FILE_NAME];
    if (fgets(input, MAX_FILE_NAME, stdin) != NULL)
    {
        input[strcspn(input, "\n")] = 0; // remove newline
        strncpy(s->file_name, input, MAX_FILE_NAME);
    }

    if (s->debug_mode)
        fprintf(stderr, "Debug: file name set to '%s'\n", s->file_name);
}

void set_unit_size(state *s)
{
    printf("Enter unit size:\n");
    char input[MAX_CHOICE_LEN];
    char *endptr;
    if (fgets(input, MAX_CHOICE_LEN, stdin) != NULL)
    {
        int unit_size = strtol(input, &endptr, 10);

        if (endptr == input || *endptr != '\n')
            fprintf(stderr, "Invalid input\n");

        else if (unit_size != 1 && unit_size != 2 && unit_size != 4)
            fprintf(stderr, "Invalid unit size\n");

        else
        {
            s->unit_size = unit_size;
            if (s->debug_mode)
                fprintf(stderr, "Debug: set size to %d\n", s->unit_size);
        }
    }
}

void load_into_memory(state *s)
{
    // check if filename is empty
    if (s->file_name[0] == '\0')
    {
        fprintf(stderr, "No file name set\n");
        return;
    }

    // open file for reading
    FILE *file = fopen(s->file_name, "r");
    if (!file)
    {
        fprintf(stderr, "Failed to open file\n");
        return;
    }

    // prompt for location in hex and length in dec
    printf("Please enter <location> <length>\n");
    char input[MAX_CHOICE_LEN];
    unsigned int location, length;
    if (fgets(input, MAX_CHOICE_LEN, stdin) != NULL)
    {
        sscanf(input, "%x %d\n", &location, &length);

        if (s->debug_mode)
            fprintf(stderr, "Debug: file name: %s, location: %#X, length: %d\n", s->file_name, location, length);

        // read from file into memory
        fseek(file, location, SEEK_SET);
        reset_mem_buf(s);
        s->mem_count = fread(s->mem_buf, s->unit_size, length, file);
        if (s->mem_count)
            fprintf(stderr, "Loaded %d units into memory\n", s->mem_count);
        else
            fprintf(stderr, "Failed to read from file\n");
    }

    fclose(file);
}

void toggle_display_mode(state *s)
{
    s->display_mode ^= 1;
    printf("Display flag now %s, %s representation\n", s->display_mode ? "on" : "off", s->display_mode ? "hexadecimal" : "decimal");
}

void memory_display(state *s)
{
    printf("Enter address and length\n");
    char input[MAX_CHOICE_LEN];
    unsigned int addr, length;
    if (fgets(input, MAX_CHOICE_LEN, stdin) != NULL)
    {
        sscanf(input, "%x %d\n", &addr, &length);

        printf(s->display_mode ? "Hexadecimal\n===========\n" : "Decimal\n=======\n");

        if (addr + (length - 1) * s->unit_size >= MAX_BUFFER_LEN)
        {
            fprintf(stderr, "Out of bounds\n");
            return;
        }

        for (int i = 0; i < length; ++i)
            printf(s->display_mode ? hex_formats[s->unit_size - 1] : dec_formats[s->unit_size - 1], *(unsigned int *)(s->mem_buf + addr + i * s->unit_size));
    }
}

void save_into_file(state *s)
{
    // check if filename is empty
    if (s->file_name[0] == '\0')
    {
        fprintf(stderr, "No file name set\n");
        return;
    }

    // open file for writing
    FILE *file = fopen(s->file_name, "r+");
    if (!file)
    {
        fprintf(stderr, "Failed to open file\n");
        return;
    }

    printf("Please enter <source-address> <target-location> <length>\n");
    char input[MAX_CHOICE_LEN];
    unsigned int source_addr, target_location, length;
    if (fgets(input, MAX_CHOICE_LEN, stdin) != NULL)
    {
        sscanf(input, "%x %x %d\n", &source_addr, &target_location, &length);

        fseek(file, 0, SEEK_END);
        if (target_location > ftell(file))
        {
            fprintf(stderr, "Target location is greater than file size\n");
            fclose(file);
            return;
        }

        if (s->debug_mode)
            fprintf(stderr, "Debug: source address: %#X, target location: %#X, length: %d\n", source_addr, target_location, length);

        fseek(file, target_location, SEEK_SET);
        fwrite(source_addr ? (void *)source_addr : s->mem_buf, s->unit_size, length, file);
    }

    fclose(file);
}

void memory_modify(state *s)
{
    printf("Please enter <location> <val>\n");
    char input[MAX_CHOICE_LEN];
    unsigned int location, val;
    if (fgets(input, MAX_CHOICE_LEN, stdin) != NULL)
    {
        sscanf(input, "%x %x\n", &location, &val);

        if (s->debug_mode)
            fprintf(stderr, "Debug: location: %#X, val: %#X\n", location, val);

        if (location + s->unit_size >= MAX_BUFFER_LEN)
        {
            fprintf(stderr, "Out of bounds\n");
            return;
        }

        *(unsigned int *)(s->mem_buf + location) = val;
        s->mem_count = max(location + s->unit_size, s->mem_count);
    }
}

void quit(state *s)
{
    printf("quitting\n");
    if (s)
        free(s);
    exit(EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
    state *s = calloc(1, sizeof(state));
    s->unit_size = 1;

    fun_desc functions[] =
        {{"Toggle Debug Mode", toggle_debug_mode},
         {"Set File Name", set_file_name},
         {"Set Unit Size", set_unit_size},
         {"Load Into Memory", load_into_memory},
         {"Toggle Display Mode", toggle_display_mode},
         {"Memory Display", memory_display},
         {"Save Into File", save_into_file},
         {"Memory Modify", memory_modify},
         {"Quit", quit}};

    int bound = sizeof(functions) / sizeof(functions[0]);
    int choice;
    char input[MAX_CHOICE_LEN];
    char *endptr;
    while (!feof(stdin))
    {
        if (s->debug_mode)
            fprintf(stderr, "unit size: %d\nfile name: %s\nmem count: %d\n", s->unit_size, s->file_name, s->mem_count);

        printf("Choose action:\n");
        for (int i = 0; i < sizeof(functions) / sizeof(functions[0]); ++i)
            printf("%d-%s\n", i, functions[i].name);

        if (fgets(input, MAX_CHOICE_LEN, stdin) != NULL)
        {
            choice = strtol(input, &endptr, 10);

            if (endptr == input || *endptr != '\n')
                // invalid input - for now we do nothing and continue
                fprintf(stderr, "Invalid input\n");

            else if (choice < 0 || choice >= bound)
                fprintf(stderr, "Not within bounds\n");

            else
                functions[choice].fun(s);
        }
    }

    return 0;
}