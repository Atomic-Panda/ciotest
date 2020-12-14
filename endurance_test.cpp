// requires c++11 and unix system

#include <iostream>
#include <cstdio>
#include <random>
#include <chrono>
#include <unistd.h>

using namespace std;

#define FILE_COUNT (32)
#define FILE_SIZE  (1024ULL * 1024 * 1024)  // 1GB for each file

#define MAX_FILE_COUNT (1024)
#define MAX_FILE_SIZE (16ULL * 1024 * 1024 * 1024)  // 16 GB

// Warning: these minimum values should not be modified
//          because some code logic relies on them
#define MIN_FILE_COUNT (1)
#define MIN_FILE_SIZE (16ULL * 1024 * 1024)  // 16 MB

int file_count = FILE_COUNT;
int file_size = FILE_SIZE;

unsigned seed = chrono::system_clock::now().time_since_epoch().count();
default_random_engine Gn(seed);
int rand_range(int x) {
	if (x <= 0) return 0;
	return uniform_int_distribution<int>(0, x - 1)(Gn);
}
int rand_range(int x, int y) {  // random integer in range [x, y)
	if (x <= 0 || x >= y) return 0;
	return uniform_int_distribution<int>(x, y - 1)(Gn);
}

void sync_file(FILE *f) {
    fflush(f);  // first flush the file to OS
    int fd = fileno(f);
    fsync(fd);  // wait until OS has writen to real disk; only works on unix based systems
}

string file_size_to_string(unsigned long long size) {
    vector<string> suffixes = {"", "K", "M", "G", "T", "P"};
    double d_size = size;
    char buf[100];
    for (int i = 0; i < suffixes.size() - 1; ++i) {
        if (d_size < 1024) {
            sprintf(buf, "%.2lf %sB", d_size, suffixes[i].c_str());
            return string(buf);
        }
        d_size /= 1024;
    }
    sprintf(buf, "%.2lf %sB", d_size, suffixes.back().c_str());
    return string(buf);
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        file_count = atoi(argv[1]);  // set file count in the first parameter (if any)
        if (file_count > MAX_FILE_COUNT) {
            printf("Error: File count should be no larger than %d\n", MAX_FILE_COUNT);
            return 1;
        } else if (file_count < MIN_FILE_COUNT) {
            printf("Error: File count should be no less than %d\n", MIN_FILE_COUNT);
            return 1;
        }
    }
    if (argc > 2) {
        file_size = atoi(argv[2]);  // set file size in the second parameter (if any)
        if (file_size > MAX_FILE_SIZE) {
            printf("Error: File size should be no larger than %s\n", file_size_to_string(MAX_FILE_SIZE).c_str());
            return 1;
        } else if (file_size < MIN_FILE_SIZE) {
            printf("Error: File size should be no less than %s\n", file_size_to_string(MIN_FILE_SIZE).c_str());
            return 1;
        }
    }

    printf("[Test Parameters]\n");
    printf("File count: %d\n", file_count);
    printf("File size: %s\n", file_size_to_string(file_size).c_str());

    printf("--- Start test ---\n");

    // initialize each file
    printf("--- Initializing files ---\n");
    FILE **files = new FILE*[file_count];
    for (int i = 0; i < file_count; ++i) {
        char file_name[30];
        sprintf(file_name, "test_file_%d", i);
        FILE *f = files[i] = fopen(file_name, "w");
        
        // fill the file with character 'a'
        string s(1024, 'a');
        for (int j = 0; j < file_size / 1024; ++j) fwrite(s.c_str(), sizeof(char), 1024, f);  
        if (file_size % 1024) {
            string s_rem(file_size % 1024, 'a');
            fwrite(s_rem.c_str(), sizeof(char), file_size % 1024, f);
        }

        sync_file(f);  // ensure the changes have been commited to real hardware

        printf("%s created.\n", file_name);
    }

    // write indefinitely to test the endurance of storage hardware
    printf("--- Performing writing test ---\n");
    auto start_time = chrono::system_clock::now();
    unsigned long long total_write_count = 0;
    unsigned long long total_write_size = 0;
    while (true) {
        // select a random file
        int file_ind = rand_range(file_count);
        FILE *f = files[file_ind];

        // generate a string with a random length and a random character
        int str_len = rand_range(1024, MIN_FILE_SIZE);  // no longer than MIN_FILE_SIZE
        string s(str_len, 'a' + rand_range(26));  // random character in the alphabet

        // write the string at a random position in the selected file
        int position = rand_range(file_size - str_len);
        fseek(f, position, SEEK_SET);  // with SEEK_SET, offset is calculated from the start of the file
        fwrite(s.c_str(), sizeof(char), str_len, f);

        sync_file(f);
        total_write_size += str_len;
        
        if (++total_write_count % 10000 == 0) {
            auto current_time = chrono::system_clock::now();
            auto duration = chrono::duration<double>(current_time - start_time);
            printf("Write count: %lld; Total write size: %s; Elapsed time: %.2lf seconds\n",
                    total_write_count, file_size_to_string(total_write_size).c_str(), duration.count());
        }
    }

    // Since the program is assumed to never end, we don't delete or clean anything here

    return 0;
}
