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