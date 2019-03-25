#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>

typedef struct{
	int row;
	int col;
	int (*sudoku)[9];
}juego;

#define NTHREADS 21

void *validateRows(void *data){
	int nums[10] = {0};
	juego *params = (juego *)data;
	int i, j;
	for(i = 0; i < 9; ++i){
		for(j = 0; j < 9; ++j){
			if(nums[params->sudoku_grid[i][j]] == 1){
				return (void *)-1; // no es correcta la fila
			}
			nums[params->sudoku_grid[i][j]] = 1;
		}
		memset(nums, 0, sizeof(int)*10);
	}
	return (void *)0; // Es correcta la fila
}


void *validateCols(void *data){
	int nums[10] = {0};
	juego *params = (juego *)data;
	int i, j;
	for(i = 0; i < 9; ++i){
		for(j = 0; j < 9; ++j){
			if(nums[params->sudoku_grid[j][i]] == 1){ 
				return (void *)-1; // no es correcta la columna
			}
			nums[params->sudoku_grid[j][i]] = 1;
		}
		memset(nums, 0, sizeof(int)*10);
	}
	return (void *)0; // la columna es correcta
}

// validar los cuadros del sudoku
void *validateSubgrid(void *data){
	int nums_check[10] = {0};
	juego *params = (juego *)data;
	int i, j;
	for(i = params->row; i < params->row + 3; ++i){
		for(j = params->col; j < params->col + 3; ++j){
			if(nums_check[params->sudoku[i][j]] == 1){
				return (void *)-1; // no es correcto el cuadro
			}
			nums_check[params->sudoku[i][j]] = 1;
		}
	}
	return (void *)0; // Es correcto el cuadro
}

int readsudokufile(int (*grid)[9], int grid_no, FILE *fp){
	int garbage;
	fseek(fp, 0, SEEK_SET);
	fscanf(fp, "%d", &garbage);
	fseek(fp, 1, SEEK_CUR); // Seek to start of first sudoku grid

	if(grid_no < 1){
		puts("Not a valid grid number. Please specify a grid number > 0.");
		return -1;
	}
	else if(grid_no > 1){ // Seek past newlines from previous grids
		fseek(fp, 9*(grid_no - 1), SEEK_CUR); // 10 newlines per grid when more than one grid
	}

	fseek(fp, (grid_no - 1)*sizeof(char)*81, SEEK_CUR); // Seek to the start of the corresponding grid number

	char entry;
	int i = 0, j = 0, totalValues = 0;
	while((fread(&entry, 1, 1, fp)) > 0 && totalValues < 81){ // Read 81 digits from file: sudoku grid 9x9 = 81
		if(entry != '\n'){ // Ignore newline
			if(isdigit(entry)){
				++totalValues;
				grid[i][j] = entry - '0'; // Store integer representation
				++j;
				if(j == 9){
					j = 0;
					++i;
				}
			}
			else{ // A non-digit character was read
				return -1;
			}
		}
	}

	return 0; // Successfully read sudoku grid from file
}

int main(int argc, char *argv[]){
	
	int sudoku[9][9];
	int n_grids;
	FILE *fp = fopen(argv[1], "r");

	fscanf(fp,"%d",&n_grids);
	fseek(fp, 1, SEEK_CUR); 

	juego *data[9];
	int row, col, i = 0;
	for(row = 0; row < 9; row += 3)
	{
		for(col = 0; col < 9; col += 3, ++i)
		{
			data[i] = (juego *)malloc(sizeof(juego));
			if(data[i] == NULL){
				int err = errno;
				puts(strerror(err));
				exit(EXIT_FAILURE);
			}
			data[i]->row = row;
			data[i]->col = col;
			data[i]->sudoku = sudoku;
		}
	}

	// vlidar usando threads
	pthread_t tid[NTHREADS];
	int g, p, j, h, retCode, check_flag = 0, t_status[NTHREADS];
	for(g = 1; g <= n_grids; ++g){
		if(readSudokuGrid(sudoku, g, fp)){
			puts("it happen reading from the file");
			exit(EXIT_FAILURE);
		}

		// threads en los cuadros 
		for(p = 0; p < 9; ++p){
			if(retCode = pthread_create(&tid[p], NULL, valgrids, (void *)data[p])){
				fprintf(stderr, "Error - pthread_create() return code: %d\n", retCode);
				exit(EXIT_FAILURE);
			}
		}


		// threads para filas y columnas
		if(retCode = pthread_create(&tid[9], NULL, valrow, (void *)data[0])){
			fprintf(stderr, "Error - pthread_create() return code: %d\n", retCode);
			exit(EXIT_FAILURE);
		}
		if(retCode = pthread_create(&tid[10], NULL, valcol, (void *)data[0])){
			fprintf(stderr, "Error - pthread_create() return code: %d\n", retCode);
			exit(EXIT_FAILURE);
		}

		// esperar que todos los threads terminen 
		for(j = 0; j < NTHREADS; ++j){
			if(retCode = pthread_join(tid[j], (void *)&t_status[j])){
				fprintf(stderr, "Error - pthread_join() return code: %d\n", retCode);
				exit(EXIT_FAILURE);
			}
		}

		// ver los estados del thread
		for(h = 0; h < NTHREADS; ++h){
			if(t_status[h]){
				check_flag = 1;
				break;
			}
		}
		if(check_flag){
			printf("Sudoku Puzzle: %d - Invalid\n", g);
		}else{
			printf("Sudoku Puzzle: %d - Valid\n", g);
		}
		check_flag = 0;
	}

	// liberar memoria y cerrar file
	int k;
	for(k = 0; k < 9; ++k){
		free(data[k]);
	}
	fclose(fp);

	return 0;
}

