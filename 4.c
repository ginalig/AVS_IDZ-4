#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

// Структура запроса обращения к БД
struct Query {
    char type;
    int key;
    int value;
};

// Структура пары
struct Pair {
    int first;
    int second;
};

sem_t sem; // Семафор для синхронизации между читателями и писателями
pthread_mutex_t mutex; // Мьютекс для блокирования доступа к r_num

int r_num = 0; // Количество читателей, которые сейчас читают БД
int db[10] = {1,2,3,4,5,6,7,8,9,10}; // База данных

char* output; // Название файла для вывода

// Функция писателя
void *writer(void *wno)
{   
    // Получаем пару из аргумента
    struct Pair pair;
    pair = *((struct Pair *)wno);
    int key = pair.first;
    int value = pair.second;
    
    // Писатель блокирует семафор, чтобы никто не мог читать или писать в БД
    sem_wait(&sem);

    // Запись в БД
    db[key] = value;
    usleep(300000);
    printf("Writer modified db[%d] to %d\n",key,value);
    FILE *fp = fopen(output, "a");
    fprintf(fp, "Writer modified db[%d] to %d\n", key, value);
    fclose(fp);
    // Писатель освобождает семафор, чтобы другие писатели и читатели могли работать с БД
    sem_post(&sem);
}

// Функция читателя
void *reader(void *rno)
{   
    // Получаем ключ из аргумента
    int key = *((int *)rno);
    int value = db[key];
    
    // Блокировка доступа к r_num
    pthread_mutex_lock(&mutex);
    
    r_num++;

    // Если это первый читатель, он блокирует семафор, чтобы писатель не мог записать в БД
    if(r_num == 1) {
        sem_wait(&sem);
    }

    // Разблокировка доступа к r_num
    pthread_mutex_unlock(&mutex);
    
    // Чтение из БД
    printf("Reader read db[%d] = %d\n", key, value);
    FILE *fp = fopen(output, "a");
    fprintf(fp, "Reader read db[%d] = %d\n", key, value);
    fclose(fp);
    usleep(300000);

    // Блокировка доступа к r_num
    pthread_mutex_lock(&mutex);
    r_num--;
    if(r_num == 0) {
        sem_post(&sem); // Если это последний читатель, он освобождает семафор
    }
    // Разблокировка доступа к r_num
    pthread_mutex_unlock(&mutex);
}

int main(int argc, char *argv[])
{
    // Инициализация семафора и мьютекса
    pthread_mutex_init(&mutex, NULL);
    sem_init(&sem,0,1);

    // Количество запросов
    int query_count;

    // Массив запросов
    struct Query *input_queries;

    // Генерация случайных запросов
    if (argv[1][0]== '-' && argv[1][1] == 'r') {
        srand(time(NULL));

        // Генерация случайного количества запросов
        query_count = rand() % 10 + 1;
        input_queries = (struct Query *) malloc(query_count * sizeof(struct Query));
        output = "random_output.txt";
        FILE *fp2 = fopen(output, "w");
        // Очистка файла для вывода
        fprintf(fp2, "");
        fclose(fp2);
        for (int i = 0; i < query_count; i++) {
            int key = rand() % 10;
            int value = rand() % 100;
            struct Query query;
            query.key = key;
            query.value = value;
            if (rand() % 2 == 0) {
                query.type = 'r';
            } else {
                query.type = 'w';
            }
            input_queries[i] = query;
        }
    } else { // Чтение запросов из файла
        // Проверка аргументов командной строки
        FILE *fp;
        fp = fopen(argv[1], "r");
        if (!fp) {
            printf("Input file not found!\n");
            return 0;
        }
        FILE *fp2 = fopen(argv[2], "w");
        // Очистка файла для вывода
        fprintf(fp2, "");
        fclose(fp2);
        // Из командной строки считываем имя файла для вывода
        output = argv[2];

        fscanf(fp, "%d", &query_count); // Считываем количество запросов из файла
        input_queries = (struct Query *) malloc(query_count * sizeof(struct Query));
        // Считываем запросы из файла
        for (int i = 0; i < query_count; i++) {
            char query;
            int key, value;
            fscanf(fp, " %c %d %d", &query, &key, &value);
            if (key > 9 || key < 0) {
                printf("Key out of range!\n");
                return 0;
            }
            input_queries[i].type = query;
            input_queries[i].key = key;
            input_queries[i].value = value;
        }
    }
    
    // Создаем потоки
    pthread_t queries[query_count];
    for (int i = 0; i < query_count; i++) {
        if (input_queries[i].type == 'r') {
            pthread_create(&queries[i], NULL, (void *)reader, (void *)&(input_queries[i].key)); // Создаем поток читателя
        } else {
            struct Pair pair;
            pair.first = input_queries[i].key;
            pair.second = input_queries[i].value;
            pthread_create(&queries[i], NULL, (void *)writer, (void *)&pair); // Создаем поток писателя
        }
    }

    // Ждем завершения потоков
    for (int i = 0; i < query_count; i++) {
        pthread_join(queries[i], NULL); 
    }
    
    // Уничтожаем семафор и мьютекс
    pthread_mutex_destroy(&mutex);
    sem_destroy(&sem);

    free(input_queries);
    return 0;
}
