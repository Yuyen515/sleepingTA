#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <semaphore.h>
#include <unistd.h>

#define NUM_OF_STUDENTS 6
#define NUM_OF_CHAIRS 3
#define NUM_OF_TA 1

void *behavior_student(void *);
void *behavior_TA(void *);

typedef struct Students
{
	int sid;   //學生id
	int times; //學生來排隊的次數
	int chair; // 學生是否佔有椅子(1:有椅子 0:沒椅子)
} STU;

struct Teacher
{
	int tid;
};

STU *student[NUM_OF_STUDENTS];

pthread_mutex_t mutex;
sem_t ta, stu;

pthread_t *thread_students;
pthread_t *thread_TA;
struct Teacher *TA;

int wait_number[NUM_OF_CHAIRS];
int student_being_served; //助教教學中的學生
int kick;				  //學生是否被趕走(1:是 0:否)

//判斷座位是否全空
int isEmpty()
{
	int i = 0;
	while (i < NUM_OF_CHAIRS)
	{
		if (wait_number[i] != 0)
		{
			return 0;
		}
		i++;
	}
	return 1;
}

//判斷座位是否全滿
int isFull()
{
	int i = 0;
	while (i < NUM_OF_CHAIRS)
	{
		if (wait_number[i] == 0)
		{
			return 0;
		}
		i++;
	}
	return 1;
}

//將學生放入座位
void enQueue(int sid)
{
	int i = 0;
	while (i < NUM_OF_CHAIRS)
	{
		if (wait_number[i] == 0)
		{
			wait_number[i] = sid;
			return;
		}
		i++;
	}
}

//移除座位中的學生
void deQueue(int sid)
{
	int i = 0;
	while (i < NUM_OF_CHAIRS)
	{
		if (wait_number[i] == sid)
		{
			wait_number[i] = 0;
			return;
		}
		i++;
	}
	printf("你要刪的學生%c不在座位中\n", student_being_served + 64);
}

//印出目前的座位狀態
void show()
{
	int i;
	printf("------------------\n");
	for (i = 0; i < NUM_OF_CHAIRS; i++)
	{
		printf("[chair] %c \n", wait_number[i] + 64);
	}
	printf("------------------\n");
}

//初始化
void init()
{
	thread_students = malloc(NUM_OF_STUDENTS * sizeof(pthread_t));
	thread_TA = malloc(NUM_OF_TA * sizeof(pthread_t));
	sem_init(&ta, 0, 1);
	sem_init(&stu, 0, 0);
	pthread_mutex_init(&mutex, NULL);
}

int random(int MIN, int MAX)
{
	int min = MIN;
	int max = MAX;
	int x = rand() % (max - min + 1) + min;
	return x;
}

//沒座位的學生嘗試搶座位
int grab_a_chair(void *stu)
{
	int i = 0;
	int j = 0;
	struct Students *new_student = (struct Students *)stu;

	//檢查所有座位中有沒有來訪次數少於要搶座位的學生
	while (i < NUM_OF_CHAIRS)
	{
		if (student[wait_number[j] - 1]->times < new_student->times)
		{

			student[wait_number[j] - 1]->chair = 0; //被搶的學生失去座位權

			printf("student %c had already queued for the %d time, so he angrily took the student %c's' seat\n", new_student->sid + 64, new_student->times, student[wait_number[j] - 1]->sid + 64);
			printf("student %c leave for a while\n", student[wait_number[j] - 1]->sid + 64);

			//新學生取得座位權
			wait_number[j] = new_student->sid;
			new_student->chair = 1;

			return 1; //如果搶到座位回傳1
		}
		j++;
		i++;
	}
	return 0; //沒搶到座位回傳0
}

//創建學生們的執行緒
void createStudent()
{
	int i;
	struct Students *student_ptr;

	for (i = 0; i < NUM_OF_STUDENTS; i++)
	{
		student[i] = malloc(sizeof(STU));
		student_ptr = student[i]; //將學生的陣列結構位址存入指標
		student[i]->sid = i + 1;  //學生ID
		student[i]->times = 0;	  //學生一開始的訪問次數為0
		student[i]->chair = 0;	  //學生一開始都不在椅子上
		pthread_create(&thread_students[i], NULL, behavior_student, (void *)student_ptr);
		/*建立執行緒，
		第一個引數為指向執行緒識別符號的指標。
		第二個引數用來設定執行緒屬性。
		第三個引數是執行緒執行函式的起始地址。
		最後一個引數是執行函式的引數。*/
	}
}

//創建助教的執行緒
void createTA()
{
	int i;

	for (i = 0; i < NUM_OF_TA; i++)
	{
		srand(time(NULL));
		TA = malloc(sizeof(struct Teacher));
		(*TA).tid = i + 1;
		pthread_create(&thread_TA[i], NULL, behavior_TA, (void *)TA);
	}
}

//學生執行緒運行的函式
void *behavior_student(void *student)
{
	struct Students *each_student = (struct Students *)student;
	while (1)
	{
		pthread_mutex_lock(&mutex); //搶互斥鎖(要搶到才能操作座位)
		printf("student %c come\n", each_student->sid + 64);
		each_student->times++; //學生來訪次數+1

		if (!isFull())
		{
			enQueue((*each_student).sid); //將學生ID放入座位
			printf("student %c sit on the chair\n", each_student->sid + 64);
			each_student->chair = 1; //學生獲得椅子1
			show();
			sem_post(&stu);
			pthread_mutex_unlock(&mutex); //釋放互斥鎖

			//如果學生的椅子被搶或是搶到助教sem就跳出迴圈
			while (sem_trywait(&ta) == -1 && (*each_student).chair == 1)
			{
			}

			//如果學生有椅子，代表他成功搶到助教，所以可以問助教問題。
			if ((*each_student).chair == 1)
			{
				student_being_served = each_student->sid;
				sleep(5);
				printf("student %c finish asking\n", (*each_student).sid + 64);
				srand(time(NULL));
				sleep(random(5, 15));
			}
			//學生沒椅子，代表椅子被搶，所以離開一陣子。
			else
			{
				srand(time(NULL));
				sleep(random(3, 10));
			}
		}
		else //如果椅子滿了
		{
			//如果有搶到椅子
			if (grab_a_chair((void *)each_student) == 1)
			{
				each_student->chair = 1;
				show();
				sem_post(&stu);
				pthread_mutex_unlock(&mutex);

				//如果學生的椅子被搶或是搶到助教sem就跳出迴圈
				while (sem_trywait(&ta) == -1 && (*each_student).chair == 1)
				{
				}

				//如果學生有椅子，代表他成功搶到助教，所以可以問助教問題。
				if ((*each_student).chair == 1)
				{
					student_being_served = each_student->sid;
					sleep(5);
					printf("student %c finish asking\n", (*each_student).sid + 64);
					srand(time(NULL));
					sleep(random(5, 15));
				}
				else
				{
					srand(time(NULL));
					sleep(random(3, 10));
				}
			}
			//如果沒搶到椅子，學生離開一陣子。
			else
			{
				pthread_mutex_unlock(&mutex);
				printf("chair full\n");
				printf("student %c leave for a while\n", (*each_student).sid + 64);
				srand(time(NULL));
				sleep(random(3, 10));
			}
		}
	}
}

//助教執行緒運行的函式
void *behavior_TA(void *TA)
{
	char *str;
	struct Teacher *each_ta = (struct Teacher *)TA;
	while (1)
	{
		sem_wait(&stu);
		pthread_mutex_lock(&mutex);
		sleep(1);
		//搶到助教的學生釋放座位
		student[student_being_served - 1]->chair = 0;
		student[student_being_served - 1]->times = 0;
		deQueue(student_being_served);
		printf("student %c is asking TA\n", student_being_served + 64);
		srand(time(NULL));
		if (random(1, 5) == 1) // random trigger the event (20% probability)
		{
			printf("Ta has meeting later,told student %c to go.\n", student_being_served + 64);
			printf("student %c leave for a while\n", student_being_served + 64);
			printf("Ta is meeting.\n");
			pthread_mutex_unlock(&mutex);
			sleep(15); // ta meeting time.
			printf("Ta finished meeting.\n");
			sem_post(&ta);
		}
		else
		{
			pthread_mutex_unlock(&mutex);
			sleep(5);
			sem_post(&ta);
		}
	}
}

void main()
{
	int i, j;
	init();
	createStudent();
	createTA();
	for (i = 0; i < NUM_OF_STUDENTS; i++)
	{
		srand(time(NULL));
		pthread_join(thread_students[i], NULL);
	}
	for (j = 0; j < NUM_OF_TA; j++)
	{
		pthread_join(thread_TA[j], NULL);
	}
}
