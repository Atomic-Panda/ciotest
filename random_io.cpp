// requires c++11 and unix system
// usage:
// ./random_io <file_count> <file_size>
// ./random_io <file_count> 4096 for 4K test
// ./random_io <file_count> <int> <unit>

#include <iostream>
#include <cstdio>
#include <random>
#include <chrono>
#include <unistd.h>
#include <fcntl.h>
#include <memory>
#include <cstring>
#include <vector>
#include <cassert>
#include <random>
#include <algorithm>

using namespace std;

#define KB (1024)
#define MB (1024 * 1024)
#define GB (1024 * 1024 * 1024)

// the max file size
#define MAX_FILE_COUNT (1024)
#define MAX_FILE_SIZE (16ULL * 1024 * 1024 * 1024)  // 16 GB

// the min file size
#define MIN_FILE_COUNT (1)
#define MIN_FILE_SIZE (4ULL * 1024)  // 4 KB

int file_count = 0;
int file_size = 0;

unsigned seed = chrono::system_clock::now().time_since_epoch().count();
default_random_engine Gn(seed);

// the rand range is [0, x-1], which is used to test random write and read
int rand_range(int x) {
	if (x <= 0) return 0;
	return uniform_int_distribution<int>(0, x - 1)(Gn);
}

// the rand range is [x, y-1]
int rand_range(int x, int y) {  // random integer in range [x, y)
	if (x <= 0 || x >= y) return 0;
	return uniform_int_distribution<int>(x, y - 1)(Gn);
}

// convert int to <int> <unit>
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
    sprintf(buf, "%.4lf %sB", d_size, suffixes.back().c_str());
    return string(buf);
}

// fsync
void sync_file(FILE *f) {
    fflush(f);  // first flush the file to OS
    int fd = fileno(f);
    fsync(fd);  // wait until OS has writen to real disk; only works on unix based systems
}

// if use UNIX IO, we do not need to flush.
void sync_file(int fileno){
    fsync(fileno);
}

// generate data to write
vector<char> data;
unsigned char RandomChar () { return ('a'+rand()%26); }
vector<char> GenerateData(std::size_t bytes)
{
    std::vector<char> data(bytes);
    generate(data.begin(), data.end(), RandomChar);
    shuffle(data.begin(), data.end(), std::mt19937{std::random_device{}()});
    return data;
}

int main(int argc, char *argv[]) {
    if(argc < 3){
        printf("need more args. Please try again\n");
        return 0;
    }
    else if(argc > 4){
        printf("Too many argc. Usage: ./main <FILE_NUM> <FILE_SIZE>\n");
        return 0;
    }
    else if (argc == 3) {
        file_count = atoi(argv[1]);  // set file count in the first parameter (if any)
        if (file_count > MAX_FILE_COUNT) {
            printf("Error: File count should be no larger than %d\n", MAX_FILE_COUNT);
            return 1;
        } else if (file_count < MIN_FILE_COUNT) {
            printf("Error: File count should be no less than %d\n", MIN_FILE_COUNT);
            return 1;
        }
        file_size = atoi(argv[2]);  // set file size in the second parameter (if any)
        if (file_size > MAX_FILE_SIZE) {
            printf("Error: File size should be no larger than %s\n", file_size_to_string(MAX_FILE_SIZE).c_str());
            return 1;
        } else if (file_size < MIN_FILE_SIZE) {
            printf("Error: File size should be no less than %s\n", file_size_to_string(MIN_FILE_SIZE).c_str());
            return 1;
        }
    }
    if (argc == 4) {
        file_count = atoi(argv[1]);  // set file count in the first parameter (if any)
        if (file_count > MAX_FILE_COUNT) {
            printf("Error: File count should be no larger than %d\n", MAX_FILE_COUNT);
            return 1;
        } else if (file_count < MIN_FILE_COUNT) {
            printf("Error: File count should be no less than %d\n", MIN_FILE_COUNT);
            return 1;
        }
        file_size = atoi(argv[2]);  // set file size in the second parameter (if any)
        string unit = (argv[3]);
        cout << unit << endl;
        if(unit == "kb"){
            file_size *= KB;
        }
        else if(unit == "mb"){
            file_size *= MB;
        }
        else if(unit == "gb"){
            file_size *= GB;
        }
        else{
            printf("Invalid unit.\n");
            return 1;
        }
        if (file_size > MAX_FILE_SIZE) {
            printf("Error: File size should be no larger than %s\n", file_size_to_string(MAX_FILE_SIZE).c_str());
            return 1;
        } else if (file_size < MIN_FILE_SIZE) {
            printf("Error: File size should be no less than %s\n", file_size_to_string(MIN_FILE_SIZE).c_str());
            return 1;
        }
    }

    printf("[Test Parameters]\n");
    // the default setting is <32, 1GB>
    printf("File count: %d\n", file_count);
    printf("File size: %s\n", file_size_to_string(file_size).c_str());

    printf("--- Start test ---\n");

    // initialize each file
    printf("--- Initializing files ---\n");
    FILE **files = new FILE*[file_count];

    // int * files_fd = new int[file_count];?
    // memset(files_fd, -1 ,sizeof(files_fd));
    // for (int i = 0; i < file_count; ++i) {
    //     char file_name[50];
    //     sprintf(file_name, "test_file_%d", i);
    //     int curr_file = files_fd[i] = open(file_name, O_APPEND|O_RDWR|O_CREAT, 00644);
    //     // fill the file with character 'a'
    //     string s(1024, 'a');
    //     for (int j = 0; j < file_size / 1024; ++j){
    //         write(curr_file, s.c_str(), 1024);
    //         close(curr_file);  
    //     }
    //     if (file_size % 1024) {
    //         string s_rem(file_size % 1024, 'a');
    //         write(curr_file ,s_rem.c_str(),file_size % 1024);
    //     }
    //     sync_file(curr_file);  // ensure the changes have been commited to real hardware
    //     printf("%s created.\n", file_name);
    // }

    for (int i = 0; i < file_count; ++i) {
        char file_name[50];
        sprintf(file_name, "../dat/test_file_%d", i);
        FILE *f = files[i] = fopen(file_name, "w");       
        // fill the file with character 'a'
        string s(1024, 'a');
        for (int j = 0; j < file_size / 1024; ++j){
            fwrite(s.c_str(), sizeof(char), 1024, f);  
        }
        if (file_size % 1024) {
            string s_rem(file_size % 1024, 'a');
            fwrite(s_rem.c_str(), sizeof(char), file_size % 1024, f);
        }
        sync_file(f);  // ensure the changes have been commited to real hardware
        printf("%s created.\n", file_name);
    }

    // write and read randomly to test the rand IO performance
    printf("--- Performing writing test ---\n");
    unsigned long long total_write_count = 0;
    unsigned long long total_write_size = 0;
    double write_duration = 0.0;
    while (1) {
        // select a random file
        data = GenerateData(file_size);
        int file_ind = rand_range(file_count);
        FILE *f = files[file_ind];

        auto start_time = chrono::system_clock::now();
        fseek(f, 0, SEEK_SET);
        fwrite(&data[0], sizeof(char), file_size, f);
        sync_file(f);
        total_write_size += file_size;

        auto current_time = chrono::system_clock::now();
        auto duration = chrono::duration<double>(current_time - start_time);
        write_duration += duration.count();

        if (++total_write_count % 100 == 0) {
            printf("Write count: %lld; Total write size: %s; Elapsed time: %.4lf seconds\n",
                    total_write_count, file_size_to_string(total_write_size).c_str(), write_duration);
            printf("Average write speed is: %s /s.\n", file_size_to_string(total_write_size/write_duration).c_str()); 
        }
    }

    // Since the program is assumed to never end, we don't delete or clean anything here

    return 0;
}

int rand_test(){
    int counter[101] = {};
    for(int i = 0; i < 10000; i++){
        counter[rand_range(100)]++;
    }
    for(int i = 0; i< 101; i++){
        cout <<i << ": ";
        for(int j = 0; j < counter[i]; j++){
            cout << "*";
        }
        cout << endl;
    }
    return 0;
}