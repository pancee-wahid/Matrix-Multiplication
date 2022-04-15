#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>

typedef struct {
    int** data;
    int r;
    int c;
} Matrix;

typedef struct {
    Matrix* mat_1;
    Matrix* mat_2;
    Matrix* result;
    int row;
    int col;
} ThreadData;

Matrix* matrix_mem_alloc(int r, int c);
Matrix* read_matrix(char* input);
void write_file(Matrix* result, char* file_name, int method);
void thread_per_matrix(Matrix* result, Matrix* mat_1, Matrix* mat_2);
void* thread_per_row(void* arg);
void* thread_per_element(void* arg);


int main(int argc, char* argv[]){
    char* input_1 = (char*)malloc(100*sizeof(char));
    char* input_2 = (char*)malloc(100*sizeof(char));
    char* output = (char*)malloc(100*sizeof(char));
    struct timeval start, stop;

    // Handling files names
    if (argc == 1){ // no file names are entered
        input_1 = "a.txt";
        input_2 = "b.txt";
        output = "c";    
    } 
    else if (argc == 4){ // file names are entered
        strcpy(input_1, argv[1]);
        strcat(input_1, ".txt");
        
        strcpy(input_2, argv[2]);
        strcat(input_2, ".txt");

        strcpy(output, argv[3]);
    } 
    else {
        printf("Invalid Arguments!");
        exit(1);
    }
    
    // Reading matrix A and B from the specified files
    Matrix* mat_1 = read_matrix(input_1);
    Matrix* mat_2 = read_matrix(input_2);
    free(input_1);
    free(input_2);

    // Check compatibility
    if (mat_1->c != mat_2->r){
        printf("Incompatible sizes!\n");
        exit(1);    
    }

    // Apply method 1 : Thread per matrix
    Matrix* result_1 = matrix_mem_alloc(mat_1->r, mat_2->c);
    gettimeofday(&start, NULL);
    thread_per_matrix(result_1, mat_1, mat_2);
    gettimeofday(&stop, NULL);
    char* out_file_1 = (char*)malloc(100*sizeof(char));
    strcpy(out_file_1, output);
    strcat(out_file_1, "_per_matrix.txt"); 
    write_file(result_1, out_file_1, 1);
    printf("Microseconds taken by thread by matrix: %lu\n", stop.tv_usec - start.tv_usec);
    free(result_1);
    free(out_file_1);

    // Apply method 2 : Thread per row
    Matrix* result_2 = matrix_mem_alloc(mat_1->r, mat_2->c);
    gettimeofday(&start, NULL);
    pthread_t rows_thread[mat_1->r]; // declare threads
    for (int i = 0; i < mat_1->r; i++){
        // pack data needed in struct
        ThreadData* thread_row_data = (ThreadData *)malloc(sizeof(ThreadData));
        thread_row_data->mat_1 = mat_1;
        thread_row_data->mat_2 = mat_2;
        thread_row_data->result = result_2;
        thread_row_data->row = i;
        // create thread for row i
        pthread_create(&rows_thread[i], NULL, thread_per_row, (void*)(thread_row_data));
    }
    // join (wait) the rows threads
    for (int i = 0; i < mat_1->r; i++) {
        pthread_join(rows_thread[i], NULL);
    }
    gettimeofday(&stop, NULL);   
    char* out_file_2 = (char*)malloc(100*sizeof(char));
    strcpy(out_file_2, output);
    strcat(out_file_2, "_per_row.txt"); 
    write_file(result_2, out_file_2, 2);
    printf("Microseconds taken by thread by row: %lu\n", stop.tv_usec - start.tv_usec);
    free(result_2);
    free(out_file_2);

    // Apply method 3 : Thread per element
    Matrix* result_3 =  matrix_mem_alloc(mat_1->r, mat_2->c);
    gettimeofday(&start, NULL);
    pthread_t elements_thread[mat_1->r][mat_2->c]; // declare threads
    for (int i = 0; i < mat_1->r; i++){
        for (int j = 0; j < mat_2->c; j++){
            // pack data needed in struct
            ThreadData* thread_element_data = (ThreadData *)malloc(sizeof(ThreadData));
            thread_element_data->mat_1 = mat_1;
            thread_element_data->mat_2 = mat_2;
            thread_element_data->result = result_2;
            thread_element_data->row = i;
            thread_element_data->col = j;
            // create thread for element[i][j]
            pthread_create(&elements_thread[i][j], NULL, 
                            thread_per_element, (void*)(thread_element_data));
        }    
    }
    // join (wait) the elements threads
    for (int i = 0; i < mat_1->r; i++) {
        for (int j = 0; j < mat_2->c; j++)
            pthread_join(elements_thread[i][j], NULL);
    }   
    gettimeofday(&stop, NULL);   
    char* out_file_3 = (char*)malloc(100*sizeof(char));
    strcpy(out_file_3, output);
    strcat(out_file_3, "_per_element.txt"); 
    write_file(result_3, out_file_3, 3);
    printf("Microseconds taken by thread by element: %lu\n", stop.tv_usec - start.tv_usec);
    free(result_3);
    free(out_file_3);

    free(mat_1);
    free(mat_2);
    free(output);
    
    return 0;
}

Matrix* matrix_mem_alloc(int r, int c){
    Matrix* mat = (Matrix*)malloc(sizeof(Matrix*));
    mat->r = r;
    mat->c = c;
    mat->data = (int**)malloc(r * sizeof(int*));
    for (int i = 0; i < r; i++)
        mat->data[i] = (int*)malloc(c * sizeof(int));
    return mat;
}

Matrix* read_matrix(char* input){
    FILE* file;
    int r,c;
    file = fopen(input, "r");
    if (file == NULL){
        printf("Can't open file %s\n", input);
        exit(1);
    }

    if(fscanf(file, "row=%d col=%d", &r, &c) != 2){
        printf("Invalid format in file %s. Can't extract number of rows and columns!\n", input);
        exit(1);
    }
    
    Matrix* matrix = matrix_mem_alloc(r, c);
    for (int i = 0; i < matrix->r; i++){
        for (int j = 0; j < matrix->c; j++){
            if(fscanf(file, "%d" ,&matrix->data[i][j]) != 1){
                printf("Invalid format in file %s. Can't read the matrix!\n", input);
                free(matrix);
                exit(1);
            }
        }
    }

    fclose(file);
    
    return matrix;
}

void write_file(Matrix* result, char* file_name, int method){
    FILE* file;
    file = fopen(file_name, "a");

    // print error msg in case of error while creating the file
    if (file == NULL){
        printf("Error in creating file!");
        exit(1);
    }

    switch (method){
        case 1:
            fprintf(file, "Method: A thread per matrix\n");
            break;
        case 2:
            fprintf(file, "Method: A thread per row\n");
            break;
        case 3:
            fprintf(file, "Method: A thread per element\n");
            break;
        default:
            printf("Error in printing! Invalid method number!\n");
            exit(1);
    }
    fprintf(file, "row=%d col=%d\n", result->r, result->c);
    int i, j;
    for (i = 0; i < result->r; i++){
        for (j = 0; j < result->c - 1; j++)
            fprintf(file, "%d ", result->data[i][j]);
     
        if (i < result->r - 1)
            fprintf(file, "%d\n", result->data[i][j]);
        else
            fprintf(file, "%d", result->data[i][j]);
    }
    
    fclose(file); // close the file
}

void thread_per_matrix(Matrix* result, Matrix* mat_1, Matrix* mat_2){
    for (int i = 0; i < result->r; i++){ //for each row
        for (int j = 0; j < result->c; j++){ // for each col
            result->data[i][j] = 0;
            for (int k = 0; k < mat_1->c; k++)
                result->data[i][j] += (mat_1->data[i][k] * mat_2->data[k][j]);
        }
    }
}

void* thread_per_row(void* arg){
    ThreadData* data = (ThreadData*) arg;
    // multiply row i of mat_1 by each column of mat_2 forming row i of result
    for (int j = 0; j < data->result->c; j++){
        data->result->data[data->row][j] = 0;
        for (int k = 0; k < data->mat_1->c; k++){
            data->result->data[data->row][j] += 
                        (data->mat_1->data[data->row][k] * data->mat_2->data[k][j]);

        }
    }
    free(data);
    pthread_exit(NULL);
}

void* thread_per_element(void* arg){
    ThreadData* data = (ThreadData*) arg;
    data->result->data[data->row][data->col] = 0;
    for (int k = 0; k < data->mat_1->c; k++){
        data->result->data[data->row][data->col] += 
                    (data->mat_1->data[data->row][k] * data->mat_2->data[k][data->col]);

    }
    free(data);
    pthread_exit(NULL);
}