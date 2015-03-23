#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

typedef struct{
	int dest;
	char mesaj[512];
}message;
int main(int argc, char **argv){
	int cnt=0;
	
	int i, j, k;
	
	int rank, size;
 	MPI_Status status;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	
	if(rank == 0){
		// citirea grafului din fisier
		char buf[512];
		FILE *in = fopen(argv[1], "r");
		if(!in) {
			printf("ERROR: while opening %s \n", argv[1]);
		}
		
		fgets(buf,512,in);
		char * p = strtok(buf," - ");
		while(cnt != rank){
			fgets(buf,512,in);
			cnt++;
		}
		int vecini[100];
		int cntLine = 0 ;
		int pos = 0 ;
		int matrix[size][size];
		for(i = 0; i < size; ++i){
			for(j = 0; j < size; ++j){
				matrix[i][j] = 0;
			}
		}
		int altmatrix[size][size];
		for(i = 0; i < size; ++i){
			for(j = 0; j < size; ++j){
				altmatrix[i][j] = 0;
			}
		}

		int finalmatrix[size][size];
		for(i = 0; i < size; ++i){
			for(j = 0; j < size; ++j){
				finalmatrix[i][j] = 0;
			}
		}

		while(p!=NULL){
			if(cntLine > 1 ){
				vecini[pos] = atoi(p);
				matrix[rank][atoi(p)] = 1;
				pos++;
			}	
			p = strtok(NULL," ");
		cntLine++;
		}
		
		printf("\n\n-----INCEPE ETAPA 1-----\n\n");

		// trimit matricea catre vecinii nodului 0
		for(i = 0; i < pos; ++i){
			MPI_Send(matrix, size*size, MPI_INT, vecini[i], 1, MPI_COMM_WORLD);
		}
		
		// primesc matricile de la copii si construiesc matricea de adiacenta
		for(k = 0; k < pos; ++k){
			MPI_Recv(altmatrix, size*size, MPI_INT, vecini[k], 2, MPI_COMM_WORLD,&status);
			for (i = 0; i < size; ++i){
				for (j = 0; j < size; ++j){
					if(altmatrix[i][j] == 1){
						finalmatrix[i][j] = 1;
					}
				}
				
			}
		}
		printf("----------MATRICEA DE ADIACENTA-----------\n\n\n");

		for (i = 0; i < size; ++i){
			for (j = 0; j < size; ++j){
				if(finalmatrix[i][j] != finalmatrix[j][i]){
					finalmatrix[i][j] = 0;
				}
				printf("%d ",finalmatrix[i][j]);
			}
			printf("\n");
		}
		printf("\n\n\n");

		//trimit matricea catre toate celelalte noduri pentru a-si crea vectorul de next_hop
		for(i = 1; i < size; ++i){
			MPI_Send(finalmatrix, size*size, MPI_INT, i, 3, MPI_COMM_WORLD);
		}

		// creez vectorul de next_hop pentru procesul cu rank 0
		int dist[size][size];
			int next_hop[size];
			for(i = 0; i < size; ++i){
				for(j = 0; j < size; ++j){
					dist[i][j] = finalmatrix[i][j];
				}
			}

			for (k = 0; k < size; ++k)
    			for (i = 0; i < size; ++i)
      				for (j = 0; j < size; ++j)
        				if (i!=j && dist[i][k] && dist[k][j] &&
            				(dist[i][j] > dist[k][j] + dist[i][k] || dist[i][j] == 0))
         					 dist[i][j] = dist[k][j] + dist[i][k];
         	printf("next_hop vector pentru rankul %d:    ",rank);
  			for (i = 0; i < size; ++i)
    			for (j = 0; j < size; ++j)
      				if (dist[j][i] == dist[rank][i]-1) {
       					next_hop[i] = j;
        				break;
     				}
     		for(i = 0; i < size; ++i){
     			next_hop[0] = -1;
     			if(rank == i){
     				next_hop[i] = -1;
     			}
     			printf("%d ,", next_hop[i]);
     		}
     		printf("\n");

     		//incep broadcasturile
     		printf("nodul %d anunta restul nodurilor ca urmeaza o comunicatie\n",rank);
     		for (j = 0; j < size; ++ j){ 
          		if(rank != j) {
          			message broadcast;
          			broadcast.dest = -2;
            		MPI_Send(&broadcast, sizeof(message), MPI_BYTE, j, 6, MPI_COMM_WORLD);
            	}					
          	}

          	for (j = 0; j < size - 1; ++j){
          		message rbroadcast;
          		MPI_Recv(&rbroadcast, sizeof(message), MPI_BYTE, MPI_ANY_SOURCE, 6, MPI_COMM_WORLD,&status);
          	} 

          	printf("\n\n-----INCEPE ETAPA 2-----\n\n");

          	//bucla de ascultare si de trimitere de la celelalte noduri
     		for(;;){
     			
    			message rmessage;
    			
    			MPI_Recv(&rmessage, sizeof(message), MPI_BYTE, MPI_ANY_SOURCE, 4, MPI_COMM_WORLD,&status);
    			if(next_hop[rmessage.dest] == -1){
    				printf("%s | a ajuns la destinatie %d\n",rmessage.mesaj,rank);
    				//break;
    			}else if(rmessage.dest == -2){
    				printf("%s --- broadcast primit de %d\n",rmessage.mesaj,rank);
    			}else{
    				printf("%s --- se trimite din nodul %d in nodul %d\n",rmessage.mesaj,rank,next_hop[rmessage.dest]);
    				MPI_Send(&rmessage, sizeof(message), MPI_BYTE, next_hop[rmessage.dest],4, MPI_COMM_WORLD);
    			}
    		}
	
	}else{
		int amatrix[size][size];
		for(i = 0; i < size; ++i){
			for(j = 0; j < size; ++j){
				amatrix[i][j] = 0;
			}
		}
		int rmatrix[size][size];
		for(i = 0; i < size; ++i){
			for(j = 0; j < size; ++j){
				rmatrix[i][j] = 0;
			}
		}

		int topmatrix[size][size];
		for(i = 0; i < size; ++i){
			for(j = 0; j < size; ++j){
				topmatrix[i][j] = 0;
			}
		}
		
		
		MPI_Recv(rmatrix, size*size, MPI_INT, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD,&status);

		int parinte = status.MPI_SOURCE;
		char buf[512];
		FILE *in = fopen(argv[1], "r");
		if(!in) {
			printf("ERROR: while opening %s \n", argv[1]);
		}
		
		fgets(buf,512,in);
		char * p = strtok(buf," - ");
		cnt = 0;
		while(cnt != rank){
			fgets(buf,512,in);
			cnt++;
		}
		int vecini[100];
		int cntLine = 0;
		int pos = 0 ;
		
		while(p!=NULL){
			if(cntLine > 1 ){
				vecini[pos] = atoi(p);
				rmatrix[rank][atoi(p)] = 1;
				pos++;
			}	
			p = strtok(NULL," ");
			cntLine++;
		}

		if(pos == 0){
				//parinte
			

		}else{
			for(i = 0; i < pos; ++i){
				if(vecini[i] != parinte){
					MPI_Send(rmatrix, size*size, MPI_INT, vecini[i], 1, MPI_COMM_WORLD);
				}
			}
			int tag = -1;
			for(i = 0; i < pos; ++i){
				if(vecini[i] != parinte){
					MPI_Recv(amatrix, size*size, MPI_INT, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD,&status);
					if(status.MPI_TAG == 1){
						rmatrix[rank][status.MPI_SOURCE] = 0;
						rmatrix[status.MPI_SOURCE][rank] = 0;
					}else{
						for(j = 0; j < size; ++j){
							for(k = 0; k < size; ++k){
								if(amatrix[j][k] == 1){
									rmatrix[j][k] = 1;
								}
							}
						}
					}

				}
			}
		}
		MPI_Send(rmatrix, size*size, MPI_INT, parinte, 2, MPI_COMM_WORLD);

		MPI_Recv(topmatrix, size*size, MPI_INT, MPI_ANY_SOURCE, 3, MPI_COMM_WORLD,&status);
		int dist[size][size];
		int next_hop[size];
		for(i = 0; i < size; ++i){
			for(j = 0; j < size; ++j){
				dist[i][j] = topmatrix[i][j];
			}
		}

		for (k = 0; k < size; ++k)
    		for (i = 0; i < size; ++i)
      			for (j = 0; j < size; ++j)
        			if (i!=j && dist[i][k] && dist[k][j] &&
           				(dist[i][j] > dist[k][j] + dist[i][k] || dist[i][j] == 0))
         				 dist[i][j] = dist[k][j] + dist[i][k];
        printf("next_hop vector pentru rankul %d:    ",rank);
  		for (i = 0; i < size; ++i)
    		for (j = 0; j < size; ++j)
      			if (dist[j][i] == dist[rank][i]-1) {
       				next_hop[i] = j;
        			break;
     			}
     	for(i = 0; i < size; ++i){
     		next_hop[0] = parinte;
     		if(rank == i){
     			next_hop[i] = -1;
     		}
     		printf("%d ,", next_hop[i]);
     	}
     	printf("\n");

     	//incep broadcasturile
     	printf("nodul %d anunta restul nodurilor ca urmeaza o comunicatie\n",rank);
     	for (j = 0; j < size; ++ j){ 
          	if(rank != j) {
          		message broadcast;
          		broadcast.dest = -2;
            	MPI_Send(&broadcast, sizeof(message), MPI_BYTE, j, 6, MPI_COMM_WORLD);
            }					
        }

        for (j = 0; j < size - 1; ++j){
          	message rbroadcast;
          	MPI_Recv(&rbroadcast, sizeof(message), MPI_BYTE, MPI_ANY_SOURCE, 6, MPI_COMM_WORLD,&status);
        } 

     	FILE *f;
     	f = fopen(argv[2],"r");
  		int n;
  		int broadcast = 1024;
  		char mesaj[512];
  		fscanf(f,"%d", &n);
  		int send_count = 0;
  		int total = n;
  		for (i = 0; i<n; ++ i) {
    		int x;
    		char y[32];
    		fscanf(f,"%d %s ",&x, y);
    		fgets(mesaj, 512, f);
    		int broad = 0;
    		mesaj[strlen(mesaj)-1] = 0;
    		if (x == rank) {
    			// daca trebuie sa trimit broadcast
      			if (y[0] == 'B') {
        			for (j = 0; j < size; ++ j){ 
          				if(rank != j) {
          					message broadcast;
          					strcpy(broadcast.mesaj,mesaj);
          					broadcast.dest = -2;
          					if(broad == 0){
          						printf("%s --- se trimite broadcast din nodul %d catre restul nodurilor\n",mesaj,rank);
            					broad = 1;
          					}
            				MPI_Send(&broadcast, sizeof(message), MPI_BYTE, j, 4, MPI_COMM_WORLD);
            					
          				}
          			}
          		//incep trimiterea unu mesaj catre destinatie prin next_hop
      			}else {

        			int dest;
        			sscanf(y," %d ", &dest);
        				
        			message m;
        			m.dest = dest;
        			strcpy(m.mesaj,mesaj);
        			MPI_Send(&m, sizeof(message), MPI_BYTE, next_hop[dest],4, MPI_COMM_WORLD);
        			printf("%s --- se trimite din nodul %d in nodul %d\n",mesaj,rank,next_hop[dest]);
      			}

      				

   			} else {
    			if (y[0] == 'B') {
    				total++;
    			}
    		}

    			
    	}
    	//bucla de ascultare si de trimitere de la celelalte noduri
    	for(;;){
    			
			message rmessage;
    		MPI_Recv(&rmessage, sizeof(message), MPI_BYTE, MPI_ANY_SOURCE, 4, MPI_COMM_WORLD,&status);
    		if(next_hop[rmessage.dest] == -1){
    			printf("%s --- A AJUNS LA DESTINATIE in nodul %d\n",rmessage.mesaj,rank);
    		}else if(rmessage.dest == -2){
    			printf("%s --- broadcast primit de %d\n",rmessage.mesaj,rank);
    		}else{
    			printf("%s --- se trimite din nodul %d in nodul %d\n",rmessage.mesaj,rank,next_hop[rmessage.dest]);
    			MPI_Send(&rmessage, sizeof(message), MPI_BYTE, next_hop[rmessage.dest],4, MPI_COMM_WORLD);
    		}
    			
    	}		
    		
		
		

	}
	MPI_Finalize();
	return 0;
}