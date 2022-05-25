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
	int sid;   //�ǥ�id
	int times; //�ǥͨӱƶ�������
	int chair; // �ǥͬO�_�����Ȥl(1:���Ȥl 0:�S�Ȥl)
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
int student_being_served; //�U�боǤ����ǥ�
int kick;				  //�ǥͬO�_�Q����(1:�O 0:�_)

//�P�_�y��O�_����
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

//�P�_�y��O�_����
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

//�N�ǥͩ�J�y��
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

//�����y�줤���ǥ�
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
	printf("�A�n�R���ǥ�%c���b�y�줤\n", student_being_served + 64);
}

//�L�X�ثe���y�쪬�A
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

//��l��
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

//�S�y�쪺�ǥ͹��շm�y��
int grab_a_chair(void *stu)
{
	int i = 0;
	int j = 0;
	struct Students *new_student = (struct Students *)stu;

	//�ˬd�Ҧ��y�줤���S���ӳX���Ƥ֩�n�m�y�쪺�ǥ�
	while (i < NUM_OF_CHAIRS)
	{
		if (student[wait_number[j] - 1]->times < new_student->times)
		{

			student[wait_number[j] - 1]->chair = 0; //�Q�m���ǥͥ��h�y���v

			printf("student %c had already queued for the %d time, so he angrily took the student %c's' seat\n", new_student->sid + 64, new_student->times, student[wait_number[j] - 1]->sid + 64);
			printf("student %c leave for a while\n", student[wait_number[j] - 1]->sid + 64);

			//�s�ǥͨ��o�y���v
			wait_number[j] = new_student->sid;
			new_student->chair = 1;

			return 1; //�p�G�m��y��^��1
		}
		j++;
		i++;
	}
	return 0; //�S�m��y��^��0
}

//�Ыؾǥ̪ͭ������
void createStudent()
{
	int i;
	struct Students *student_ptr;

	for (i = 0; i < NUM_OF_STUDENTS; i++)
	{
		student[i] = malloc(sizeof(STU));
		student_ptr = student[i]; //�N�ǥͪ��}�C���c��}�s�J����
		student[i]->sid = i + 1;  //�ǥ�ID
		student[i]->times = 0;	  //�ǥͤ@�}�l���X�ݦ��Ƭ�0
		student[i]->chair = 0;	  //�ǥͤ@�}�l�����b�Ȥl�W
		pthread_create(&thread_students[i], NULL, behavior_student, (void *)student_ptr);
		/*�إ߰�����A
		�Ĥ@�Ӥ޼Ƭ����V������ѧO�Ÿ������СC
		�ĤG�Ӥ޼ƥΨӳ]�w������ݩʡC
		�ĤT�Ӥ޼ƬO���������禡���_�l�a�}�C
		�̫�@�Ӥ޼ƬO����禡���޼ơC*/
	}
}

//�ЫاU�Ъ������
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

//�ǥͰ�����B�檺�禡
void *behavior_student(void *student)
{
	struct Students *each_student = (struct Students *)student;
	while (1)
	{
		pthread_mutex_lock(&mutex); //�m������(�n�m��~��ާ@�y��)
		printf("student %c come\n", each_student->sid + 64);
		each_student->times++; //�ǥͨӳX����+1

		if (!isFull())
		{
			enQueue((*each_student).sid); //�N�ǥ�ID��J�y��
			printf("student %c sit on the chair\n", each_student->sid + 64);
			each_student->chair = 1; //�ǥ���o�Ȥl1
			show();
			sem_post(&stu);
			pthread_mutex_unlock(&mutex); //���񤬥���

			//�p�G�ǥͪ��Ȥl�Q�m�άO�m��U��sem�N���X�j��
			while (sem_trywait(&ta) == -1 && (*each_student).chair == 1)
			{
			}

			//�p�G�ǥͦ��Ȥl�A�N��L���\�m��U�СA�ҥH�i�H�ݧU�а��D�C
			if ((*each_student).chair == 1)
			{
				student_being_served = each_student->sid;
				sleep(5);
				printf("student %c finish asking\n", (*each_student).sid + 64);
				srand(time(NULL));
				sleep(random(5, 15));
			}
			//�ǥͨS�Ȥl�A�N��Ȥl�Q�m�A�ҥH���}�@�}�l�C
			else
			{
				srand(time(NULL));
				sleep(random(3, 10));
			}
		}
		else //�p�G�Ȥl���F
		{
			//�p�G���m��Ȥl
			if (grab_a_chair((void *)each_student) == 1)
			{
				each_student->chair = 1;
				show();
				sem_post(&stu);
				pthread_mutex_unlock(&mutex);

				//�p�G�ǥͪ��Ȥl�Q�m�άO�m��U��sem�N���X�j��
				while (sem_trywait(&ta) == -1 && (*each_student).chair == 1)
				{
				}

				//�p�G�ǥͦ��Ȥl�A�N��L���\�m��U�СA�ҥH�i�H�ݧU�а��D�C
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
			//�p�G�S�m��Ȥl�A�ǥ����}�@�}�l�C
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

//�U�а�����B�檺�禡
void *behavior_TA(void *TA)
{
	char *str;
	struct Teacher *each_ta = (struct Teacher *)TA;
	while (1)
	{
		sem_wait(&stu);
		pthread_mutex_lock(&mutex);
		sleep(1);
		//�m��U�Ъ��ǥ�����y��
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
